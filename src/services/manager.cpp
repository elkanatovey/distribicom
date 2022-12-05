#include "manager.hpp"
#include <grpc++/grpc++.h>
#include "utils.hpp"


namespace services {
// i want the manager to have a map with running tasks. these tasks should be some kind of struc with a channel waiting to be
// fulfilled... the map can be refered to using a round, and an epoch as its key.
// upon completion of a task it should go back to the map and state all workers have freaking completed their tasks.

    std::uint64_t query_count_per_worker(std::uint64_t num_workers, std::uint64_t num_rows, std::uint64_t num_queries);


    ::grpc::Status
    Manager::RegisterAsWorker(::grpc::ServerContext *context,
                              const ::distribicom::WorkerRegistryRequest *request,
                              ::distribicom::Ack *response) {
        try {

            auto requesting_worker = utils::extract_ipv4(context);
            std::string subscribing_worker_address = requesting_worker + ":" + std::to_string(request->workerport());

            // creating client to the worker:
            auto worker_conn = std::make_unique<distribicom::Worker::Stub>(distribicom::Worker::Stub(
                    grpc::CreateChannel(
                            subscribing_worker_address,
                            grpc::InsecureChannelCredentials()
                    )
            ));

            auto add = 0;

            mtx.lock();
            if (worker_stubs.find(requesting_worker) == worker_stubs.end()) {
                worker_stubs.insert({requesting_worker, std::move(worker_conn)});
                add = 1;
            }
            mtx.unlock();

            worker_counter.add(add);

        } catch (std::exception &e) {
            std::cout << "Error: " << e.what() << std::endl;
            return {grpc::StatusCode::INTERNAL, e.what()};
        }

        return {};
    }

    ::grpc::Status Manager::ReturnLocalWork(::grpc::ServerContext *context,
                                            ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                                            ::distribicom::Ack *response) {
        auto sending_worker = utils::extract_ipv4(context);

        mtx.lock();
        auto exists = worker_stubs.find(sending_worker) != worker_stubs.end();
        mtx.unlock();

        if (!exists) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "worker not registered"};
        }

        auto round = utils::extract_size_from_metadata(context->client_metadata(), constants::round_md);
        auto epoch = utils::extract_size_from_metadata(context->client_metadata(), constants::epoch_md);

        mtx.lock();
        exists = ledgers.find({round, epoch}) != ledgers.end();
        auto ledger = ledgers[{round, epoch}];
        mtx.unlock();

        if (!exists) {
            return {
                    grpc::StatusCode::INVALID_ARGUMENT,
                    "ledger not found in {round,epoch}" + std::to_string(round) + "," + std::to_string(epoch)
            };
        }

        // TODO: should verify the incoming data - corresponding to the expected {ctx, row, col} from each worker.
        distribicom::MatrixPart tmp;
        while (reader->Read(&tmp)) {
            int row = tmp.row();
            int col = tmp.col();

#ifdef DISTRIBICOM_DEBUG
            matops->w_evaluator->evaluator->sub_inplace(
                    ledger->result_mat(row, col),
                    marshal->unmarshal_seal_object<seal::Ciphertext>(tmp.ctx().data()));
            assert(ledger->result_mat(row, col).is_transparent());
#endif
        }

        ledger->mtx.lock();
        ledger->contributed.insert(sending_worker);
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
                             const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                             int rnd, int epoch,
#ifdef DISTRIBICOM_DEBUG
                             const seal::GaloisKeys &expansion_key
#endif
    ) {
        // TODO: should set up a `promise` channel specific to the round and channel, anyone requesting info should get
        //   it through the channel.
        grpc::ClientContext context;

        utils::add_metadata_size(context, services::constants::size_md, int(db.cols));
        utils::add_metadata_size(context, services::constants::round_md, rnd);
        utils::add_metadata_size(context, services::constants::epoch_md, epoch);

        // currently, there is no work distribution. everyone receives the entire DB and the entire queries.
        std::lock_guard<std::mutex> lock(mtx);
        auto ledger = std::make_shared<WorkDistributionLedger>();
        ledgers.insert({{rnd, epoch}, ledger});
        ledger->worker_list = std::vector<std::string>();
        ledger->worker_list.reserve(worker_stubs.size());
        ledger->result_mat = math_utils::matrix<seal::Ciphertext>(
                db.cols, compressed_queries.data.size());
        for (auto &worker: worker_stubs) {
            ledger->worker_list.push_back(worker.first);
        }

#ifdef DISTRIBICOM_DEBUG
        create_res_matrix(db, compressed_queries, expansion_key, ledger);
#endif

        sendtask(db, compressed_queries, context, ledger);
        return ledger;
    }


    void Manager::create_res_matrix(const math_utils::matrix<seal::Plaintext> &db,
                                    const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                                    const seal::GaloisKeys &expansion_key,
                                    std::shared_ptr<WorkDistributionLedger> &ledger) const {
#ifdef DISTRIBICOM_DEBUG
        auto exp = expansion_key;
        auto expand_sz = db.cols;
        math_utils::matrix<seal::Ciphertext> query_mat(expand_sz, compressed_queries.data.size());
        auto col = -1;
        for (auto &c_query: compressed_queries.data) {
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
                      grpc::ClientContext &context) {

        auto marshaled_db_vec = marshal->marshal_seal_vector(db.data);
        auto marshalled_db = math_utils::matrix<std::string>(db.rows, db.cols, std::move(marshaled_db_vec));

        // TODO: write into map


        distribicom::Ack response;
        distribicom::WorkerTaskPart part;
        std::unique_lock lock(mtx);

        for (auto &worker: worker_stubs) {
            { // this stream is released at the end of this scope.
                auto stream = worker.second->SendTask(&context, &response);
                auto db_rows = worker_name_to_work_responsible_for[worker.first].db_rows;

                for(const auto &db_row: db_rows) {
                    // first send db
                    for (int j = 0; j < int(db.cols); ++j) {
                        std::string payload = marshalled_db(db_row, j);

                        part.mutable_matrixpart()->set_row(db_row);
                        part.mutable_matrixpart()->set_col(j);
                        part.mutable_matrixpart()->mutable_ptx()->set_data(payload);

                        stream->Write(part);
                        part.mutable_matrixpart()->clear_ptx();
                    }
                }
                stream->WritesDone();
                auto status = stream->Finish();
                if (!status.ok()) {
                    std::cout << "manager:: distribute_work:: transmitting db to " << worker.first <<
                              " failed: " << status.error_message() << std::endl;
                    continue;
                }
            }
        }
    }



    void
    Manager::send_queries( const ClientDB &all_clients,
                          grpc::ClientContext &context) {

        // TODO: write into map


        distribicom::Ack response;
        distribicom::WorkerTaskPart part;
        std::shared_lock client_db_lock(all_clients.mutex);
        std::unique_lock lock(mtx);
        for (auto &worker: worker_stubs) {
            { // this stream is released at the end of this scope.
                auto stream = worker.second->SendTask(&context, &response);
                auto range_start = worker_name_to_work_responsible_for[worker.first].query_range_start;
                auto  range_end = worker_name_to_work_responsible_for[worker.first].query_range_end;

                for (std::uint64_t i = range_start; i < range_end; ++i) { //@todo cuurently assume that query has one ctext in dim
                    auto payload = all_clients.id_to_info.at(i)->query_info_marshaled.query_dim1(0);

                    part.mutable_matrixpart()->set_row(0);
                    part.mutable_matrixpart()->set_col(i);
                    part.mutable_matrixpart()->mutable_ctx()->CopyFrom(payload);

                    stream->Write(part);
                    part.mutable_matrixpart()->clear_ctx();

                }

                stream->WritesDone();
                auto status = stream->Finish();
                if (!status.ok()) {
                    std::cout << "manager:: distribute_work:: transmitting db to " << worker.first <<
                              " failed: " << status.error_message() << std::endl;
                    continue;
                }
            }
        }
    }


    std::shared_ptr<WorkDistributionLedger>
    Manager::sendtask(const math_utils::matrix<seal::Plaintext> &db,
                      const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                      grpc::ClientContext &context, std::shared_ptr<WorkDistributionLedger> ledger) {

        // TODO: write into map


        distribicom::Ack response;
        distribicom::WorkerTaskPart part;
        for (auto &worker: worker_stubs) {
            { // this stream is released at the end of this scope.
                auto stream = worker.second->SendTask(&context, &response);
                for (int i = 0; i < int(db.rows); ++i) {
                    for (int j = 0; j < int(db.cols); ++j) {
                        std::string payload = marshal->marshal_seal_object(db(i, j));

                        part.mutable_matrixpart()->set_row(i);
                        part.mutable_matrixpart()->set_col(j);
                        part.mutable_matrixpart()->mutable_ptx()->set_data(payload);

                        stream->Write(part);
                        part.mutable_matrixpart()->clear_ptx();
                    }
                }

                for (int i = 0; i < int(compressed_queries.data.size()); ++i) {

                    std::string payload = marshal->marshal_seal_object(compressed_queries.data[i]);
                    part.mutable_matrixpart()->set_row(0);
                    part.mutable_matrixpart()->set_col(i);
                    part.mutable_matrixpart()->mutable_ctx()->set_data(payload);

                    stream->Write(part);
                    part.mutable_matrixpart()->clear_ctx();

                }

                stream->WritesDone();
                auto status = stream->Finish();
                if (!status.ok()) {
                    std::cout << "manager:: distribute_work:: transmitting db to " << worker.first <<
                              " failed: " << status.error_message() << std::endl;
                    continue;
                }
            }
        }
        return ledger;
    }

    void Manager::wait_for_workers(int i) {
        worker_counter.wait_for(i);
    }

    void Manager::send_galois_keys(const ClientDB &all_clients) {
        grpc::ClientContext context;

        std::shared_lock client_db_lock(all_clients.mutex);
        auto num_clients = all_clients.client_counter;
        auto num_rows = app_configs.configs().db_rows();

        std::unique_lock lock(mtx);
        auto num_queries_per_worker = query_count_per_worker(worker_stubs.size(), num_rows, num_clients);

        utils::add_metadata_size(context, services::constants::size_md, int(num_queries_per_worker));
        // todo: specify epoch!
        utils::add_metadata_size(context, services::constants::round_md, 1);
        utils::add_metadata_size(context, services::constants::epoch_md, 1);

        distribicom::Ack response;
        distribicom::WorkerTaskPart prt;

        for (auto &worker: worker_stubs) {
            { // this stream is released at the end of this scope.
                auto stream = worker.second->SendTask(&context, &response);
                auto range_start = worker_name_to_work_responsible_for[worker.first].query_range_start;
                auto  range_end = worker_name_to_work_responsible_for[worker.first].query_range_end;

                for (std::uint64_t i = range_start; i < range_end; ++i) {
                    prt.mutable_gkey()->CopyFrom(all_clients.id_to_info.at(i)->galois_keys_marshaled);
                    stream->Write(prt);
                }
                stream->WritesDone();
                auto status = stream->Finish();
                if (!status.ok()) {
                    std::cout << "manager:: distribute_work:: transmitting galois gal_key to " << worker.first <<
                              " failed: " << status.error_message() << std::endl;
                    continue;
                }
            }
        }
    }

    void Manager::send_galois_keys(const math_utils::matrix<seal::GaloisKeys> &matrix) {
        grpc::ClientContext context;

        utils::add_metadata_size(context, services::constants::size_md, int(matrix.cols));
        // todo: specify epoch!
        utils::add_metadata_size(context, services::constants::round_md, 1);
        utils::add_metadata_size(context, services::constants::epoch_md, 1);

        distribicom::Ack response;
        distribicom::WorkerTaskPart prt;
        std::unique_lock lock(mtx);

        for (auto &worker: worker_stubs) {
            { // this stream is released at the end of this scope.
                auto stream = worker.second->SendTask(&context, &response);
                for (int i = 0; i < int(matrix.cols); ++i) {
                    prt.mutable_gkey()->set_key_pos(i);
                    prt.mutable_gkey()->mutable_keys()->assign(
                            marshal->marshal_seal_object(matrix(0, i))
                    );
                    stream->Write(prt);
                }
                stream->WritesDone();
                auto status = stream->Finish();
                if (!status.ok()) {
                    std::cout << "manager:: distribute_work:: transmitting galois gal_key to " << worker.first <<
                              " failed: " << status.error_message() << std::endl;
                    continue;
                }
            }
        }
    }

    std::vector<std::uint64_t> get_row_id_to_work_with(std::uint64_t id, std::uint64_t num_rows){
        auto row_id = id%num_rows;
        return {row_id};
    }

    std::pair<std::uint64_t, std::uint64_t> get_query_range_to_work_with(std::uint64_t worker_id, std::uint64_t num_queries, std::uint64_t num_queries_per_worker){
        auto range_id = worker_id / num_queries;
        auto range_start = num_queries_per_worker * range_id;
        auto range_end = range_start + num_queries_per_worker;
        return {range_start, range_end};
    }

    std::uint64_t query_count_per_worker(std::uint64_t num_workers, std::uint64_t num_rows, std::uint64_t num_queries){
#ifdef DISTRIBICOM_DEBUG
        if(num_workers==1){
            std::cout<<"Manager: using only 1 worker"<<std::endl;
            return num_queries;}
#endif

        auto num_workers_per_row = num_workers/num_rows;
        auto num_queries_per_worker = num_queries / num_workers_per_row;
        return num_queries_per_worker;
    }

    void Manager::map_workers_to_responsibilities(std::uint64_t num_rows,  std::uint64_t num_queries) {
        int i = 0;
        std::unique_lock lock(mtx);
        auto num_queries_per_worker = query_count_per_worker(worker_stubs.size(), num_rows, num_queries);

        for(auto &worker: worker_stubs){
            this->worker_name_to_work_responsible_for[worker.first].worker_number = i;
            this->worker_name_to_work_responsible_for[worker.first].db_rows = get_row_id_to_work_with(i, num_rows);
#ifdef DISTRIBICOM_DEBUG
            if(worker_stubs.size()==1){
                std::vector<std::uint64_t> temp(num_rows);
                for(int j=0; j<num_rows; j++){
                    temp[j] =j;
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

            i+=1;
        }
    }


}