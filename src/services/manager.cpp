#include "manager.hpp"
#include <grpc++/grpc++.h>
#include "utils.hpp"


namespace services {
// i want the manager to have a map with running tasks. these tasks should be some kind of struc with a channel waiting to be
// fulfilled... the map can be refered to using a round, and an epoch as its key.
// upon completion of a task it should go back to the map and state all workers have freaking completed their tasks.

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

}