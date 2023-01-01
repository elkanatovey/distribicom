#include "manager.hpp"
#include <grpc++/grpc++.h>
#include "utils.hpp"

#define UNUSED(x) (void)(x)

namespace services {

    ::grpc::Status Manager::ReturnLocalWork(::grpc::ServerContext *context,
                                            ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                                            ::distribicom::Ack *resp) {
        UNUSED(resp);
        std::string worker_creds = utils::extract_string_from_metadata(
            context->client_metadata(),
            constants::credentials_md
        );

        mtx.lock_shared();
        auto exists = work_streams.find(worker_creds) != work_streams.end();
        mtx.unlock_shared();

        if (!exists) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "worker not registered"};
        }

        distribicom::MatrixPart tmp;
        auto parts = std::make_shared<std::vector<ResultMatPart>>();

        auto &parts_vec = *parts;
        while (reader->Read(&tmp)) {
            // TODO: should verify the incoming data - corresponding to the expected {ctx, row, col} from each worker.
            auto current_ctx = marshal->unmarshal_seal_object<seal::Ciphertext>(tmp.ctx().data());
            parts_vec.push_back(
                {
                    std::move(current_ctx),
                    tmp.row(),
                    tmp.col()
                }
            );
        }

        #ifndef LOCALHOST_TEST
        async_verify_worker(parts, worker_creds);
        #endif

        put_in_result_matrix(parts_vec, this->client_query_manager);

        auto ledger = epoch_data.ledger;

        ledger->mtx.lock();
        ledger->contributed.insert(worker_creds);
        auto n_contributions = ledger->contributed.size();
        ledger->mtx.unlock();

        // signal done:
        if (n_contributions == ledger->worker_list.size()) {
            ledger->done.close();
        }

        return {};
    }

    std::shared_ptr<WorkDistributionLedger>
    Manager::distribute_work(const math_utils::matrix<seal::Plaintext> &db,
                             const ClientDB &all_clients,
                             int rnd, int epoch
    ) {
        epoch_data.ledger = new_ledger(db, all_clients);

        if (rnd == 0) {
            grpc::ClientContext context;
            utils::add_metadata_size(context, services::constants::round_md, rnd);
            utils::add_metadata_size(context, services::constants::epoch_md, epoch);

            send_queries(all_clients);
        }

        send_db(db, rnd, epoch);

        return epoch_data.ledger;
    }

    std::shared_ptr<WorkDistributionLedger>
    Manager::new_ledger(const math_utils::matrix<seal::Plaintext> &db, const ClientDB &all_clients) {
        auto ledger = std::make_shared<WorkDistributionLedger>();

        #ifndef LOCALHOST_TESTING
        for (size_t i = 0; i < epoch_data.num_freivalds_groups; i++) {
            // need to compute DB X epoch_data.query_matrix.
            matops->multiply(db, *epoch_data.query_mat_times_randvec[i], ledger->db_x_queries_x_randvec[i]);
            matops->from_ntt(ledger->db_x_queries_x_randvec[i].data);
        }
        #endif

        ledger->worker_list = std::vector<std::string>();

        std::shared_lock lock(mtx);
        ledger->worker_list.reserve(work_streams.size());
        for (auto &worker: work_streams) {
            ledger->worker_list.push_back(worker.first);
            ledger->worker_verification_results.insert(
                {
                    worker.first,
                    std::make_unique<concurrency::promise<bool>>(1, nullptr)
                }
            );
        }

        return ledger;
    }


    void Manager::create_res_matrix(const math_utils::matrix<seal::Plaintext> &db,
                                    const ClientDB &all_clients,
                                    const seal::GaloisKeys &expansion_key) const {
#ifdef DISTRIBICOM_DEBUG
        auto ledger = epoch_data.ledger;
        auto exp = expansion_key;
        auto expand_sz = db.cols;

        math_utils::matrix<seal::Ciphertext> query_mat(expand_sz, all_clients.client_counter);
        auto col = -1;
        for (const auto &client: all_clients.id_to_info) {
            auto c_query = client.second->query[0][0];
            col++;
            std::vector<seal::Ciphertext> quer(1);
            quer[0] = c_query;
            auto expanded = expander->expand_query(quer, expand_sz, exp);

            for (uint64_t i = 0; i < expanded.size(); ++i) {
                query_mat(i, col) = expanded[i];
            }
        }

        matops->to_ntt(query_mat.data);

        math_utils::matrix<seal::Ciphertext> res;
        matops->multiply(db, query_mat, res);
        ledger->result_mat = res;
#endif
    }

    void
    Manager::send_db(const math_utils::matrix<seal::Plaintext> &db,
                     int rnd, int epoch) {

        auto marshaled_db_vec = marshal->marshal_seal_vector(db.data);
        auto marshalled_db = math_utils::matrix<std::string>(db.rows, db.cols, std::move(marshaled_db_vec));

        distribicom::Ack response;
        std::shared_lock lock(mtx);

        auto latch = std::make_shared<concurrency::safelatch>(work_streams.size());
        for (auto &[name, stream]: work_streams) {

            pool->submit(
                {
                    .f = [&, name, stream]() {
                        auto part = std::make_unique<distribicom::WorkerTaskPart>();
                        part->mutable_md()->set_round(rnd);
                        part->mutable_md()->set_epoch(epoch);
                        stream->add_task_to_write(std::move(part));

                        auto db_rows = epoch_data.worker_to_responsibilities[name].db_rows;
                        for (const auto &db_row: db_rows) {
                            // first send db
                            for (std::uint32_t j = 0; j < db.cols; ++j) {
                                part = std::make_unique<distribicom::WorkerTaskPart>();

                                part->mutable_matrixpart()->set_row(db_row);
                                part->mutable_matrixpart()->set_col(j);
                                part->mutable_matrixpart()->mutable_ptx()->set_data(marshalled_db(db_row, j));

                                stream->add_task_to_write(std::move(part));
                            }
                        }

                        part = std::make_unique<distribicom::WorkerTaskPart>();
                        part->set_task_complete(true);
                        stream->add_task_to_write(std::move(part));

                        stream->write_next();
                    },
                    .wg = latch,
                }
            );
        }

        latch->wait();
    }


    void
    Manager::send_queries(const ClientDB &all_clients) {

        std::shared_lock lock(mtx);

        auto latch = std::make_shared<concurrency::safelatch>(work_streams.size());
        for (auto &[name, stream]: work_streams) {
            pool->submit(
                {
                    .f=[&, name, stream]() {
                        auto current_worker_info = epoch_data.worker_to_responsibilities[name];
                        auto range_start = current_worker_info.query_range_start;
                        auto range_end = current_worker_info.query_range_end;

                        for (std::uint64_t i = range_start;
                             i < range_end; ++i) { //@todo currently assume that query has one ctext in dim
//                auto payload = all_clients.id_to_info.at(i)->query_info_marshaled.query_dim1(0).data();

                            auto part = std::make_unique<distribicom::WorkerTaskPart>();
                            part->mutable_matrixpart()->set_row(0);
                            part->mutable_matrixpart()->set_col(i);
                            part->mutable_matrixpart()->mutable_ctx()->set_data(
                                all_clients.id_to_info.at(i)->query_info_marshaled.query_dim1(0).data());

                            stream->add_task_to_write(std::move(part));
                        }

                        auto part = std::make_unique<distribicom::WorkerTaskPart>();
                        part->set_task_complete(true);
                        stream->add_task_to_write(std::move(part));

                        stream->write_next();
                    },
                    .wg = latch
                }
            );
        }
        latch->wait();
    }


    void Manager::wait_for_workers(int i) {
        worker_counter.wait_for(i);
    }

    void Manager::send_galois_keys(const ClientDB &all_clients) {
        grpc::ClientContext context;


        utils::add_metadata_size(context, services::constants::round_md, 1);
        utils::add_metadata_size(context, services::constants::epoch_md, 1);

        std::shared_lock lock(mtx);

        auto latch = std::make_shared<concurrency::safelatch>(work_streams.size());
        for (auto &[name, stream]: work_streams) {
            pool->submit(
                {
                    .f = [&, name, stream]() {
                        auto range_start = epoch_data.worker_to_responsibilities[name].query_range_start;
                        auto range_end = epoch_data.worker_to_responsibilities[name].query_range_end;

                        for (std::uint64_t i = range_start; i < range_end; ++i) {
                            auto prt = std::make_unique<distribicom::WorkerTaskPart>();
                            prt->mutable_gkey()->CopyFrom(all_clients.id_to_info.at(i)->galois_keys_marshaled);
                            stream->add_task_to_write(std::move(prt));

                        }
                        stream->write_next();
                    },
                    .wg = latch
                }
            );
        }

        latch->wait();
    }

    std::vector<std::uint64_t> get_row_ids_to_work_with(std::uint64_t id, std::uint64_t num_rows, size_t num_workers) {
        auto row_id = id % num_rows;
        if (num_workers >= num_rows) {
            return {row_id};
        } else {
            auto num_rows_per_worker = num_rows / num_workers;
            auto row_differential = num_rows / num_rows_per_worker;
            std::vector<std::uint64_t> rows_responsible_for;
            auto curr = row_id;
            while (curr < num_rows) {
                rows_responsible_for.push_back(curr);
                curr += row_differential;
            }
            return rows_responsible_for;
        }
    }

    std::pair<std::uint64_t, std::uint64_t>
    get_query_range_to_work_with(std::uint64_t worker_id, std::uint64_t num_queries,
                                 std::uint64_t num_queries_per_worker) {
        auto range_id = worker_id / num_queries;
        auto range_start = num_queries_per_worker * range_id;
        auto range_end = range_start + num_queries_per_worker;
        return {range_start, range_end};
    }

    std::uint64_t query_count_per_worker(std::uint64_t num_workers, std::uint64_t num_rows, std::uint64_t num_queries) {
#ifdef DISTRIBICOM_DEBUG
        if (num_workers == 1) {
            std::cout << "Manager: using only 1 worker" << std::endl;
            return num_queries;
        }
#endif

        auto num_workers_per_row = num_workers / num_rows;
        if (num_workers_per_row ==
            0) { num_workers_per_row = 1; } // @todo find nicer way to handle fewer workers than rows
        auto num_queries_per_worker = num_queries / num_workers_per_row;
        return num_queries_per_worker;
    }

    std::map<std::string, WorkerInfo> Manager::map_workers_to_responsibilities2(std::uint64_t num_queries) {
        // Assuming more workers than rows.
        std::uint64_t num_groups = thread_unsafe_compute_number_of_groups();

        auto num_workers_in_group = work_streams.size() / num_groups;

        // num groups is the amount of duplication of the DB.
        auto num_queries_per_group = num_queries / num_groups; // assume num_queries>=num_groups

        auto num_rows_per_worker = app_configs.configs().db_rows() / num_workers_in_group;
        if (num_rows_per_worker == 0) {
            num_rows_per_worker = 1;
        }

        std::map<std::string, WorkerInfo> worker_to_responsibilities;
        std::uint64_t i = 0;
        std::uint64_t group_id = -1;
        for (auto const &[worker_name, stream]: work_streams) {
            (void) stream; // not using val.

            if (i % num_workers_in_group == 0) {
                group_id++;
            }
            auto worker_id = i++;

            // group_id determines that part of queries each worker receives.
            auto range_start = group_id * num_queries_per_group;
            auto range_end = range_start + num_queries_per_group;

            // worker should get a range of all rows:
            // I want each worker to have a sequential range of rows for each worker.

            std::vector<std::uint64_t> db_rows;
            db_rows.reserve(num_rows_per_worker);

            auto partition_start = (worker_id % num_workers_in_group) * num_rows_per_worker;
            for (std::uint64_t j = 0; j < num_rows_per_worker; ++j) {
                db_rows.push_back(j + partition_start);
            }

            worker_to_responsibilities[worker_name] = WorkerInfo{
                .worker_number = worker_id,
                .group_number = group_id,
                .query_range_start= range_start,
                .query_range_end = range_end,
                .db_rows = db_rows
            };
        }

        return worker_to_responsibilities;
    }

    std::uint64_t Manager::thread_unsafe_compute_number_of_groups() const {
        return std::uint64_t(std::max(size_t(1), work_streams.size() / app_configs.configs().db_rows()));
    }

    std::map<std::string, WorkerInfo> Manager::map_workers_to_responsibilities(std::uint64_t num_queries) {
        std::uint64_t num_rows = app_configs.configs().db_rows();
#ifdef DISTRIBICOM_DEBUG
        auto num_workers_per_row = work_streams.size() / num_rows;
        //multiple query bucket support dependant on freivalds
        if (num_workers_per_row > 1) { throw std::invalid_argument("currently we do not support multiple query buckets"); }
        if (num_rows % work_streams.size() != 0) {
            throw std::invalid_argument("each worker must get the same number of rows");
        }
#endif

        std::uint32_t i = 0;
        std::shared_lock lock(mtx);

        auto num_queries_per_worker = query_count_per_worker(work_streams.size(), num_rows, num_queries);
        std::map<std::string, WorkerInfo> worker_name_to_work_responsible_for;
        for (auto &worker: work_streams) {
            worker_name_to_work_responsible_for[worker.first].worker_number = i;
            worker_name_to_work_responsible_for[worker.first].db_rows = get_row_ids_to_work_with(i, num_rows,
                                                                                                 work_streams.size());
#ifdef DISTRIBICOM_DEBUG
            if (work_streams.size() == 1) {
                std::vector<std::uint64_t> temp(num_rows);
                for (std::uint32_t j = 0; j < num_rows; j++) {
                    temp[j] = j;
                }
                worker_name_to_work_responsible_for[worker.first].db_rows = std::move(temp);
                worker_name_to_work_responsible_for[worker.first].query_range_start = 0;
                worker_name_to_work_responsible_for[worker.first].query_range_end = num_queries;
                return worker_name_to_work_responsible_for;
            }
#endif
            auto query_range = get_query_range_to_work_with(i, num_queries, num_queries_per_worker);

            worker_name_to_work_responsible_for[worker.first].query_range_start = query_range.first;
            worker_name_to_work_responsible_for[worker.first].query_range_end = query_range.second;

            i += 1;
        }
        return worker_name_to_work_responsible_for;
    }

    ::grpc::ServerWriteReactor<::distribicom::WorkerTaskPart> *
    Manager::RegisterAsWorker(::grpc::CallbackServerContext *ctx, const ::distribicom::WorkerRegistryRequest *rqst) {
        UNUSED(rqst);
        std::string creds = utils::extract_string_from_metadata(ctx->client_metadata(), constants::credentials_md);
        // Assuming for now, no one leaves !
        auto stream = new WorkStream();

        mtx.lock();
        work_streams[creds] = stream;
        mtx.unlock();

        worker_counter.add(1);

        return stream;
    }

    void randomise_scalar_vec(std::vector<std::uint64_t> &vec) {
        seal::Blake2xbPRNGFactory factory;
        auto prng = factory.create({(std::random_device()) ()});
        std::uniform_int_distribution<unsigned long long> dist(
            std::numeric_limits<uint64_t>::min(),
            std::numeric_limits<uint64_t>::max()
        );

        for (auto &i: vec) { i = prng->generate(); }
    }

    void Manager::new_epoch(const ClientDB &db) {
        auto num_freivalds_groups = thread_unsafe_compute_number_of_groups();
        auto size_freivalds_group = db.client_counter / num_freivalds_groups;

        EpochData ed{
            .worker_to_responsibilities = map_workers_to_responsibilities2(db.client_counter),
            .queries = {}, // todo: consider removing this (not sure we need to store the queries after this func.
            .queries_dim2 = {},
            .random_scalar_vector = std::make_shared<std::vector<std::uint64_t>>(size_freivalds_group),
            .query_mat_times_randvec = {},
            .num_freivalds_groups = num_freivalds_groups,
            .size_freivalds_group = size_freivalds_group,
        };

        auto expand_size = app_configs.configs().db_cols();
        auto expand_size_dim2 = app_configs.configs().db_rows();

        // promises for expanded queries
        #ifndef LOCALHOST_TESTING
        std::vector<std::shared_ptr<concurrency::promise<std::vector<seal::Ciphertext>>>> qs(db.id_to_info.size());
        #endif
        std::vector<std::shared_ptr<concurrency::promise<math_utils::matrix<seal::Ciphertext>>>> qs2(
            db.id_to_info.size());

        for (const auto &info: db.id_to_info) {
            #ifndef LOCALHOST_TESTING
            qs[info.first] = expander->async_expand(
                info.second->query[0],
                expand_size,
                info.second->galois_keys
            );
            #endif

            // setting dim2 in matrix<seal::ciphertext> and ntt form.
            auto p = std::make_shared<concurrency::promise<math_utils::matrix<seal::Ciphertext>>>(1, nullptr);
            pool->submit(
                {
                    .f = [&, p]() {
                        auto mat = std::make_shared<math_utils::matrix<seal::Ciphertext>>(
                            1, expand_size_dim2, // row vec.
                            expander->expand_query(
                                info.second->query[1],
                                expand_size_dim2,
                                info.second->galois_keys
                            ));
                        matops->to_ntt(mat->data);
                        p->set(mat);
                    },
                    .wg  = p->get_latch(),
                }
            );
            qs2[info.first] = p;

        }

        #ifndef LOCALHOST_TESTING // Freivalds-preprocessing
        randomise_scalar_vec(*ed.random_scalar_vector);

        auto rows = expand_size;
        std::vector<std::unique_ptr<concurrency::promise<math_utils::matrix<seal::Ciphertext>>>> promises;

        std::uint64_t current_query_group = 0;
        while (current_query_group < ed.num_freivalds_groups) {

            auto current_query_mat = std::make_shared<math_utils::matrix<seal::Ciphertext>>(rows,
                                                                                            ed.size_freivalds_group);

            auto range_start = current_query_group * ed.size_freivalds_group;
            auto range_end = range_start + ed.size_freivalds_group;
            for (std::uint64_t column = range_start; column < range_end; column++) {
                auto v = qs[column]->get();
                ed.queries[column] = v;

                for (std::uint64_t i = 0; i < rows; i++) {
                    (*current_query_mat)(i, column - range_start) = (*v)[i];
                }
            }

            promises.push_back(matops->async_scalar_dot_product(
                current_query_mat,
                ed.random_scalar_vector
            ));

            current_query_group++;
        }
        for (size_t i = 0; i < ed.num_freivalds_groups; i++) {
            ed.query_mat_times_randvec[i] = promises[i]->get();
            matops->to_ntt(ed.query_mat_times_randvec[i]->data);
        }
        qs.clear();
        #endif

        for (std::uint64_t column = 0; column < qs2.size(); column++) {
            ed.queries_dim2[column] = qs2[column]->get();
        }
        qs2.clear();

        mtx.lock();
        epoch_data = std::move(ed);
        mtx.unlock();
    }

    void Manager::wait_on_verification() {
        for (const auto &v: epoch_data.ledger->worker_verification_results) {
            auto is_valid = *(v.second->get());
            if (!is_valid) {
                throw std::runtime_error("wait_on_verification:: invalid verification");
            }
        }
    }

    void Manager::put_in_result_matrix(const std::vector<ResultMatPart> &parts, ClientDB &all_clients) {

        all_clients.mutex->lock_shared();
        for (const auto &partial_answer: parts) {
            math_utils::EmbeddedCiphertext ptx_embedding;
            this->matops->w_evaluator->get_ptx_embedding(partial_answer.ctx, ptx_embedding);
            this->matops->w_evaluator->transform_to_ntt_inplace(ptx_embedding);
            for (size_t i = 0; i < ptx_embedding.size(); i++) {
                (*all_clients.id_to_info[partial_answer.col]->partial_answer)(partial_answer.row, i) = std::move(
                    ptx_embedding[i]);
            }
            all_clients.id_to_info[partial_answer.col]->answer_count += 1;
        }
        all_clients.mutex->unlock_shared();
    }

    void Manager::calculate_final_answer() {
        for (const auto &client: client_query_manager.id_to_info) {
            auto current_query = &epoch_data.queries_dim2[client.first];
            matops->mat_mult(**current_query, (*client.second->partial_answer), (*client.second->final_answer));
            matops->from_ntt(client.second->final_answer->data);
        }
    }

    bool
    Manager::verify_row(std::shared_ptr<math_utils::matrix<seal::Ciphertext>> &workers_db_row_x_query,
                        std::uint64_t row_id,
                        std::uint64_t group_id) {
        try {
            auto challenge_vec = epoch_data.random_scalar_vector;

            auto db_row_x_query_x_challenge_vec = matops->scalar_dot_product(workers_db_row_x_query, challenge_vec);
            auto expected_result = epoch_data.ledger->db_x_queries_x_randvec[group_id].data[row_id];

            matops->w_evaluator->evaluator->sub_inplace(db_row_x_query_x_challenge_vec->data[0], expected_result);
            return db_row_x_query_x_challenge_vec->data[0].is_transparent();
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    void

    Manager::async_verify_worker(const std::shared_ptr<std::vector<ResultMatPart>> parts_ptr,
                                 const std::string worker_creds) {
        pool->submit(
            {
                .f=[&, parts_ptr, worker_creds]() {
                    auto &parts = *parts_ptr;

                    auto work_responsibility = epoch_data.worker_to_responsibilities[worker_creds];
                    auto rows = work_responsibility.db_rows;
                    auto query_row_len =
                        work_responsibility.query_range_end - work_responsibility.query_range_start;

#ifdef DISTRIBICOM_DEBUG
                    if (query_row_len != epoch_data.size_freivalds_group) {
                        throw std::runtime_error(
                            "unimplemented case: query_row_len != epoch_data.size_freivalds_group");
                    }
#endif
                    for (size_t i = 0; i < rows.size(); i++) {
                        std::vector<seal::Ciphertext> temp;
                        temp.reserve(query_row_len);
                        for (size_t j = 0; j < query_row_len; j++) {
                            temp.push_back(parts[j + i * query_row_len].ctx);
                        }

                        auto workers_db_row_x_query = std::make_shared<math_utils::matrix<seal::Ciphertext>>(
                            query_row_len, 1,
                            temp);
                        auto is_valid = verify_row(workers_db_row_x_query, rows[i],
                                                   work_responsibility.group_number);
                        if (!is_valid) {
                            epoch_data.ledger->worker_verification_results[worker_creds]->set(
                                std::make_unique<bool>(false)
                            );
                            return;
                        }
                    }

                    epoch_data.ledger->worker_verification_results[worker_creds]->set(std::make_unique<bool>(true));
                },
                .wg = epoch_data.ledger->worker_verification_results[worker_creds]->get_latch()
            }
        );

    }


}