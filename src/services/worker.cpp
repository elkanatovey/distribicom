#include "worker.hpp"
#include "utils.hpp"
#include "distribicom.pb.h"
#include <grpc++/grpc++.h>

class NotImplemented : public std::logic_error {
public:
    NotImplemented() : std::logic_error("Function not yet implemented") {};

    explicit NotImplemented(const std::string &txt) : std::logic_error(txt) {};
};
namespace services {

    // todo: take a const ref of the app_configs:
    Worker::Worker(distribicom::WorkerConfigs &&wcnfgs)
            : symmetric_secret_key(), cnfgs(std::move(wcnfgs)), chan(), mrshl() {
        inspect_configs();
        seal::EncryptionParameters enc_params = utils::setup_enc_params(cnfgs.appconfigs());

        mrshl = marshal::Marshaller::Create(enc_params);


        // todo: put in a different function.
        auto manager_conn = std::make_unique<distribicom::Manager::Stub>(distribicom::Manager::Stub(
                grpc::CreateChannel(
                        cnfgs.appconfigs().main_server_hostname(),
                        grpc::InsecureChannelCredentials()
                )
        ));

        grpc::ClientContext context;
        distribicom::Ack response;
        distribicom::WorkerRegistryRequest request;

        request.set_workerport(cnfgs.workerport());

        strategy = std::make_shared<work_strategy::RowMultiplicationStrategy>(
                enc_params,
                mrshl, std::move(manager_conn)
        );

        threads.emplace_back(
                [&]() {
                    std::cout << "worker main thread: running" << std::endl;
                    for (;;) {
                        auto task = chan.read();
                        if (!task.ok) {
                            std::cout << "worker main thread: stopping execution" << std::endl;
                            break;
                        }
                        strategy->process_task(std::move(task.answer));
                    }
                });

        setup();
    }

    void Worker::inspect_configs() const {
        if (cnfgs.appconfigs().configs().scheme() != "bgv") {
            throw std::invalid_argument("currently not supporting any scheme oother than bgv.");
        }

        auto row_size = cnfgs.appconfigs().configs().db_cols();
        if (row_size <= 0) {
            throw std::invalid_argument("row size must be positive");
        }
        if (row_size > 10 * 1000 || row_size <= 0) {
            throw std::invalid_argument("row size must be between 1 and 10k");
        }
    }

    grpc::Status
    Worker::SendTask(grpc::ServerContext *context, grpc::ServerReader<::distribicom::WorkerTaskPart> *reader,
                     distribicom::Ack *_) {
        try {
            WorkerServiceTask task(context, cnfgs.appconfigs().configs());

            int n_matrix_parts = 0;

            distribicom::WorkerTaskPart tmp;
            while (reader->Read(&tmp)) {

                switch (tmp.part_case()) {
                    case distribicom::WorkerTaskPart::PartCase::kGkey:
                        strategy->store_galois_key(
                                mrshl->unmarshal_seal_object<seal::GaloisKeys>(tmp.gkey().keys()),
                                int(tmp.gkey().key_pos())
                        );

                        break;

                    case distribicom::WorkerTaskPart::PartCase::kMatrixPart:
                        n_matrix_parts += 1;
                        fill_matrix_part(task, tmp.matrixpart());

                        break;

                    default:
                        throw std::invalid_argument("received a message that is not a matrix part or a gkey");

                }

                tmp.clear_part();
            }

            if (n_matrix_parts != 0) {
                chan.write(task);
            }

        } catch (std::invalid_argument &e) {
            std::cout << "Worker::SendTask::Exception: " << e.what() << std::endl;
            return {grpc::StatusCode::INVALID_ARGUMENT, e.what()};
        } catch (std::exception &e) {
            std::cout << "Worker::SendTask:: " << e.what() << std::endl;
            return {grpc::StatusCode::INTERNAL, e.what()};
        }

        return {};
    }

    void Worker::fill_matrix_part(WorkerServiceTask &task, const distribicom::MatrixPart &tmp) const {
        int row = tmp.row();
        int col = tmp.col();

        if (tmp.has_ptx()) {
            if (!task.ptx_rows.contains(row)) {
                task.ptx_rows[row] = std::vector<seal::Plaintext>(task.row_size);
            }
            if (col >= task.row_size) {
                throw std::invalid_argument("Invalid column index, too big");
            }
            task.ptx_rows[row][col]
                    = mrshl->unmarshal_seal_object<seal::Plaintext>(tmp.ptx().data());
            return;
        }

        if (!tmp.has_ctx()) {
            throw std::invalid_argument("Invalid matrix part, neither ptx nor ctx");
        }

        if (!task.ctx_cols.contains(col)) {
            task.ctx_cols[col] = std::vector<seal::Ciphertext>(1);
        }

        task.ctx_cols[col][0] = mrshl->unmarshal_seal_object<seal::Ciphertext>(tmp.ctx().data());
    }

    void Worker::close() {
        chan.close();
        for (auto &t: threads) {
            t.join();
        }
    }

    WorkerServiceTask::WorkerServiceTask(grpc::ServerContext *pContext, const distribicom::Configs &configs) :
            epoch(-1), round(-1), row_size(int(configs.db_cols())), ptx_rows(), ctx_cols() {

        const auto &metadata = pContext->client_metadata();
        epoch = utils::extract_size_from_metadata(metadata, constants::epoch_md);
        round = utils::extract_size_from_metadata(metadata, constants::round_md);

        if (epoch < 0) {
            throw std::invalid_argument("epoch must be positive");
        }
        if (round < 0) {
            throw std::invalid_argument("round must be positive");
        }
    }

    void Worker::setup() {
//        threads.emplace_back(std::thread([&] {
            auto stub = distribicom::Manager::NewStub(grpc::CreateChannel(
                    cnfgs.appconfigs().main_server_hostname(),
                    grpc::InsecureChannelCredentials()
            ));

            class Reader : public grpc::ClientReadReactor<distribicom::WorkerTaskPart> {
            public:
                Reader(std::unique_ptr<distribicom::Manager::Stub> &&stub) : stub(std::move(stub)) {
                    distribicom::WorkerRegistryRequest rqst;
                    this->stub->async()->RegisterAsWorker(&context_, &rqst, this);

                    StartRead(&tsk_); // queueing a read request.
                    StartCall();
                }

                void OnReadDone(bool ok) override {
                    if (ok) {
                        std::cout << "READ my first thing yo!";
                    }

                    StartRead(&tsk_);// queue the next read request.
                }

                // call Await to receive Finish from server.
                void OnDone(const grpc::Status &s) override {
                    std::unique_lock<std::mutex> l(mu_);
                    status_ = s;
                    done_ = true;
                    cv_.notify_one();
                }

                grpc::Status Await() {
                    std::unique_lock<std::mutex> l(mu_);
                    cv_.wait(l, [this] { return done_; });
                    return std::move(status_);
                }

            private:
                std::unique_ptr<distribicom::Manager::Stub> stub;
                grpc::ClientContext context_;
                distribicom::WorkerTaskPart tsk_;
                std::mutex mu_;
                std::condition_variable cv_;
                grpc::Status status_;
                bool done_ = false;
            };

            Reader reader(std::move(stub));
            reader.Await();
//        }));
//        sleep(2);
    }
}

