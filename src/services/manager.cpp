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
        return Service::ReturnLocalWork(context, reader, response);
    }

    std::unique_ptr<WorkDistributionLedger>
    Manager::distribute_work(const math_utils::matrix<seal::Plaintext> &db,
                             const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                             int rnd, int epoch) {
        // TODO: should set up a `promise` channel specific to the round and channel, anyone requesting info should get
        //   it through the channel.
        grpc::ClientContext context;

        utils::add_metadata_size(context, services::constants::size_md, int(db.cols));
        utils::add_metadata_size(context, services::constants::round_md, rnd);
        utils::add_metadata_size(context, services::constants::epoch_md, epoch);

        // currently, there is no work distribution. everyone receives the entire DB and the entire queries.
        std::lock_guard<std::mutex> lock(mtx);

        return sendtask(db, compressed_queries, context);
    }

    std::unique_ptr<WorkDistributionLedger>
    Manager::sendtask(const math_utils::matrix<seal::Plaintext> &db,
                      const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                      grpc::ClientContext &context) {

        auto ledger = std::make_unique<WorkDistributionLedger>();
        ledger->worker_list = std::vector<std::string>(worker_stubs.size());
        ledger->result_mat = math_utils::matrix<seal::Ciphertext>(
                db.cols, compressed_queries.data.size());

        distribicom::Ack response;
        distribicom::WorkerTaskPart part;
        for (auto &worker: worker_stubs) {
            { // this stream is released at the end of this scope.
                auto stream = worker.second->SendTask(&context, &response);
                for (int i = 0; i < int(db.rows); ++i) {
                    for (int j = 0; i < int(db.cols); ++i) {
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
                ledger->worker_list.push_back(worker.first);
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