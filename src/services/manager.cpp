#include "manager.hpp"
#include <grpc++/grpc++.h>
#include "utils.hpp"


namespace services {

    ::grpc::Status Manager::ReturnLocalWork(::grpc::ServerContext *context,
                                            ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                                            ::distribicom::Ack *response) {
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

        auto round = utils::extract_size_from_metadata(context->client_metadata(), constants::round_md);
        auto epoch = utils::extract_size_from_metadata(context->client_metadata(), constants::epoch_md);

        mtx.lock_shared();
        exists = ledgers.find({round, epoch}) != ledgers.end();
        auto ledger = ledgers[{round, epoch}];
        mtx.unlock_shared();

        if (!exists) {
            return {
                    grpc::StatusCode::INVALID_ARGUMENT,
                    "ledger not found in {round,epoch}" + std::to_string(round) + "," + std::to_string(epoch)
            };
        }

        // TODO: should verify the incoming data - corresponding to the expected {ctx, row, col} from each worker.
        distribicom::MatrixPart tmp;
        while (reader->Read(&tmp)) {
            std::uint32_t row = tmp.row();
            std::uint32_t col = tmp.col();

#ifdef DISTRIBICOM_DEBUG
            matops->w_evaluator->evaluator->sub_inplace(
                    ledger->result_mat(row, col),
                    marshal->unmarshal_seal_object<seal::Ciphertext>(tmp.ctx().data()));
            assert(ledger->result_mat(row, col).is_transparent());
#endif
        }

        ledger->mtx.lock();
        ledger->contributed.insert(worker_creds);
        auto n_contributions = ledger->contributed.size();
        ledger->mtx.unlock();

        // signal done:
        if (n_contributions == ledger->worker_list.size()) {
            ledger->done.close();
        }

        // mat = matrix<n,j>;
        // while (reader->Read(&tmp)) {
        //      tmp == [ctx, 0, j]
        //       mat[0,j] = ctx;;
        // }

        // A
        // B
        // C

        // Frievalds(A, B, C)
        // Frievalds: DB[:, :]

        return {};
    }

    std::shared_ptr<WorkDistributionLedger>
    Manager::distribute_work(const math_utils::matrix<seal::Plaintext> &db,
                             const ClientDB &all_clients,
                             int rnd, int epoch,
#ifdef DISTRIBICOM_DEBUG
                             const seal::GaloisKeys &expansion_key
#endif
    ) {

        auto ledger = std::make_shared<WorkDistributionLedger>();
        {
            // currently, there is no work distribution. everyone receives the entire DB and the entire queries.
            std::shared_lock lock(mtx);
            ledgers.insert({{rnd, epoch}, ledger});
            ledger->worker_list = std::vector<std::string>();
            ledger->worker_list.reserve(work_streams.size());
            all_clients.mutex.lock_shared();
            ledger->result_mat = math_utils::matrix<seal::Ciphertext>(
                    db.cols, all_clients.client_counter);
            all_clients.mutex.unlock_shared();
            for (auto &worker: work_streams) {
                ledger->worker_list.push_back(worker.first);
            }
        }
#ifdef DISTRIBICOM_DEBUG
        create_res_matrix(db, all_clients, expansion_key, ledger);
#endif
        if (rnd == 1) {
            grpc::ClientContext context;
            utils::add_metadata_size(context, services::constants::round_md, rnd);
            utils::add_metadata_size(context, services::constants::epoch_md, epoch);

            send_queries(all_clients);
        }

        send_db(db, rnd, epoch);

        return ledger;
    }


    void Manager::create_res_matrix(const math_utils::matrix<seal::Plaintext> &db,
                                    const ClientDB &all_clients,
                                    const seal::GaloisKeys &expansion_key,
                                    std::shared_ptr<WorkDistributionLedger> &ledger) const {
#ifdef DISTRIBICOM_DEBUG
        auto exp = expansion_key;
        auto expand_sz = db.cols;

        all_clients.mutex.lock_shared();
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
        all_clients.mutex.unlock_shared();

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

            auto db_rows = worker_name_to_work_responsible_for[worker.first].db_rows;
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

        std::shared_lock client_db_lock(all_clients.mutex);
        std::shared_lock lock(mtx);
        for (auto &worker: work_streams) {

            auto current_worker_info = worker_name_to_work_responsible_for[worker.first];
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

        std::shared_lock client_db_lock(all_clients.mutex);
        std::shared_lock lock(mtx);

        for (auto &worker: work_streams) {

            auto range_start = worker_name_to_work_responsible_for[worker.first].query_range_start;
            auto range_end = worker_name_to_work_responsible_for[worker.first].query_range_end;

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


    void Manager::map_workers_to_responsibilities(std::uint64_t num_rows, std::uint64_t num_queries) {
        std::uint32_t i = 0;
        std::shared_lock lock(mtx);

        auto num_queries_per_worker = query_count_per_worker(worker_stubs.size(), num_rows, num_queries);

        for (auto &worker: worker_stubs) {
            this->worker_name_to_work_responsible_for[worker.first].worker_number = i;
            this->worker_name_to_work_responsible_for[worker.first].db_rows = get_row_id_to_work_with(i, num_rows);
#ifdef DISTRIBICOM_DEBUG
            if (worker_stubs.size() == 1) {
                std::vector<std::uint64_t> temp(num_rows);
                for (std::uint32_t j = 0; j < num_rows; j++) {
                    temp[j] = j;
                }
                this->worker_name_to_work_responsible_for[worker.first].db_rows = std::move(temp);
                this->worker_name_to_work_responsible_for[worker.first].query_range_start = 0;
                this->worker_name_to_work_responsible_for[worker.first].query_range_end = num_queries;
                return;
            }
#endif
            auto query_range = get_query_range_to_work_with(i, num_queries, num_queries_per_worker);

            this->worker_name_to_work_responsible_for[worker.first].query_range_start = query_range.first;
            this->worker_name_to_work_responsible_for[worker.first].query_range_end = query_range.second;

            i += 1;
        }
    }

    ::grpc::ServerWriteReactor<::distribicom::WorkerTaskPart> *
    Manager::RegisterAsWorker(::grpc::CallbackServerContext *ctx, const ::distribicom::WorkerRegistryRequest *rqst) {

        std::string creds = utils::extract_string_from_metadata(ctx->client_metadata(), constants::credentials_md);
        // Assuming for now, no one leaves !
        auto stream = new WorkStream();

        mtx.lock();
        work_streams[creds] = stream;
        mtx.unlock();

        worker_counter.add(1);

        return stream;
    }


}