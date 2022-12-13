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

        async_verify_worker(parts, worker_creds);
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
        #ifdef DISTRIBICOM_DEBUG
        , const seal::GaloisKeys &expansion_key
        #endif
    ) {
        epoch_data.ledger = new_ledger(db, all_clients);

        #ifdef DISTRIBICOM_DEBUG
        create_res_matrix(db, all_clients, expansion_key);
        #endif

        if (rnd == 1) {
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

        ledger->worker_list = vector<string>();
        ledger->worker_list.reserve(work_streams.size());
        ledger->result_mat = math_utils::matrix<seal::Ciphertext>(db.cols, all_clients.client_counter);

        // need to compute DB X epoch_data.query_matrix.
        matops->multiply(db, *epoch_data.query_mat_times_randvec, ledger->db_x_queries_x_randvec);
        matops->from_ntt(ledger->db_x_queries_x_randvec.data);

        shared_lock lock(mtx);
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

        for (auto &worker: work_streams) {
            auto part = std::make_unique<distribicom::WorkerTaskPart>();
            part->mutable_md()->set_round(rnd);
            part->mutable_md()->set_epoch(epoch);

            worker.second->add_task_to_write(std::move(part));

            auto db_rows = epoch_data.worker_to_responsibilities[worker.first].db_rows;
            for (const auto &db_row: db_rows) {
                // first send db
                for (std::uint32_t j = 0; j < db.cols; ++j) {
                    part = std::make_unique<distribicom::WorkerTaskPart>();

                    part->mutable_matrixpart()->set_row(db_row);
                    part->mutable_matrixpart()->set_col(j);
                    part->mutable_matrixpart()->mutable_ptx()->set_data(marshalled_db(db_row, j));

                    worker.second->add_task_to_write(std::move(part));
                }
            }

            part = std::make_unique<distribicom::WorkerTaskPart>();
            part->set_task_complete(true);
            worker.second->add_task_to_write(std::move(part));

            worker.second->write_next();
        }
    }


    void
    Manager::send_queries(const ClientDB &all_clients) {

        std::shared_lock lock(mtx);
        for (auto &worker: work_streams) {

            auto current_worker_info = epoch_data.worker_to_responsibilities[worker.first];
            auto range_start = current_worker_info.query_range_start;
            auto range_end = current_worker_info.query_range_end;

            for (std::uint64_t i = range_start;
                 i < range_end; ++i) { //@todo currently assume that query has one ctext in dim
                auto payload = all_clients.id_to_info.at(i)->query_info_marshaled.query_dim1(0);

                auto part = std::make_unique<distribicom::WorkerTaskPart>();
                part->mutable_matrixpart()->set_row(0);
                part->mutable_matrixpart()->set_col(i);
                part->mutable_matrixpart()->mutable_ctx()->CopyFrom(payload);

                worker.second->add_task_to_write(std::move(part));
            }

            auto part = std::make_unique<distribicom::WorkerTaskPart>();
            part->set_task_complete(true);
            worker.second->add_task_to_write(std::move(part));

            worker.second->write_next();
        }
    }


    void Manager::wait_for_workers(int i) {
        worker_counter.wait_for(i);
    }

    void Manager::send_galois_keys(const ClientDB &all_clients) {
        grpc::ClientContext context;


        utils::add_metadata_size(context, services::constants::round_md, 1);
        utils::add_metadata_size(context, services::constants::epoch_md, 1);

        std::shared_lock lock(mtx);

        for (auto &worker: work_streams) {

            auto range_start = epoch_data.worker_to_responsibilities[worker.first].query_range_start;
            auto range_end = epoch_data.worker_to_responsibilities[worker.first].query_range_end;

            for (std::uint64_t i = range_start; i < range_end; ++i) {
                auto prt = std::make_unique<distribicom::WorkerTaskPart>();
                prt->mutable_gkey()->CopyFrom(all_clients.id_to_info.at(i)->galois_keys_marshaled);
                worker.second->add_task_to_write(std::move(prt));

            }
            worker.second->write_next();
        }

    }

    std::vector<std::uint64_t> get_row_id_to_work_with(std::uint64_t id, std::uint64_t num_rows) {
        auto row_id = id % num_rows;
        return {row_id};
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
        auto num_queries_per_worker = num_queries / num_workers_per_row;
        return num_queries_per_worker;
    }


    map<string, WorkerInfo> Manager::map_workers_to_responsibilities(std::uint64_t num_queries) {
        std::uint64_t num_rows = app_configs.configs().db_rows();
        std::uint32_t i = 0;
        std::shared_lock lock(mtx);

        auto num_queries_per_worker = query_count_per_worker(work_streams.size(), num_rows, num_queries);
        std::map<std::string, WorkerInfo> worker_name_to_work_responsible_for;
        for (auto &worker: work_streams) {
            worker_name_to_work_responsible_for[worker.first].worker_number = i;
            worker_name_to_work_responsible_for[worker.first].db_rows = get_row_id_to_work_with(i, num_rows);
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
        auto prng = factory.create({(random_device()) ()});
        uniform_int_distribution<unsigned long long> dist(
            numeric_limits<uint64_t>::min(),
            numeric_limits<uint64_t>::max()
        );

        for (auto &i: vec) { i = prng->generate(); }
    }

    void Manager::new_epoch(const ClientDB &db) {
        EpochData ed{
            .worker_to_responsibilities = map_workers_to_responsibilities(db.client_counter),
            .queries = {}, // todo: consider removing this (not sure we need to store the queries after this func.
            .queries_dim2 = {},
            .random_scalar_vector = std::make_shared<std::vector<std::uint64_t>>(db.client_counter),
            .query_mat_times_randvec = {},
        };

        auto expand_size = app_configs.configs().db_cols();
        auto expand_size_dim2 = app_configs.configs().db_rows();

        std::vector<std::shared_ptr<concurrency::promise<std::vector<seal::Ciphertext>>>> qs(db.id_to_info.size());
        std::vector<std::shared_ptr<concurrency::promise<math_utils::matrix<seal::Ciphertext>>>> qs2(
            db.id_to_info.size());

        for (const auto &info: db.id_to_info) {
            qs[info.first] = expander->async_expand(
                info.second->query[0],
                expand_size,
                info.second->galois_keys
            );

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

        randomise_scalar_vec(*ed.random_scalar_vector);

        auto rows = expand_size;
        auto query_mat = std::make_shared<math_utils::matrix<seal::Ciphertext>>(rows, db.client_counter);

        for (std::uint64_t column = 0; column < qs.size(); column++) {
            auto v = qs[column]->get();
            ed.queries[column] = v;

            for (std::uint64_t i = 0; i < rows; i++) {
                (*query_mat)(i, column) = (*v)[i];
            }
        }
        qs.clear();

        auto promise = matops->async_scalar_dot_product(
            query_mat,
            ed.random_scalar_vector
        );

        for (std::uint64_t column = 0; column < qs2.size(); column++) {
            ed.queries_dim2[column] = qs2[column]->get();
        }
        qs2.clear();

        ed.query_mat_times_randvec = promise->get();
        matops->to_ntt(ed.query_mat_times_randvec->data);

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
}