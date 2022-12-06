#include "worker.hpp"
#include "utils.hpp"
#include "distribicom.pb.h"
#include <grpc++/grpc++.h>

#include "worker_reader.hpp"

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
                        auto task_ = chan.read();
                        if (!task_.ok) {
                            std::cout << "worker main thread: stopping execution" << std::endl;
                            break;
                        }
                        strategy->process_task(std::move(task_.answer));
                    }
                });

        setup_stream();
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

    void Worker::update_current_task() {
        auto tmp = read_val.matrixpart();
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


    void Worker::OnReadDone(bool ok) {
        try {
            if (!ok) {
                // bad read might mean data too big.
                // bad read might mean closed stream.
                // todo: find proper way to find out the error!
                throw std::runtime_error("bad read");
            }

            switch (read_val.part_case()) {
                case distribicom::WorkerTaskPart::PartCase::kGkey:
                    std::cout << "writing galois keys" << std::endl;
                    strategy->store_galois_key(
                            mrshl->unmarshal_seal_object<seal::GaloisKeys>(read_val.gkey().keys()),
                            int(read_val.gkey().key_pos())
                    );

                    break;

                case distribicom::WorkerTaskPart::PartCase::kMatrixPart:
                    update_current_task();
                    break;

                default:
                    if (!task.ptx_rows.empty() || !task.ctx_cols.empty()) {
                        std::cout << "sending task to be processed" << std::endl;
                        chan.write(std::move(task));
                        task = WorkerServiceTask();
                        task.row_size = int(cnfgs.appconfigs().configs().db_cols());
                    }
            }
        } catch (std::exception &e) {
            read_val.clear_part();
            read_val.clear_gkey();
            read_val.clear_matrixpart();
            std::cout << "Worker::OnReadDone: failure: " << e.what() << std::endl;
        }

        StartRead(&read_val);// queue the next read request.
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

    void Worker::setup_stream() {
        // TODO: setup any value that we need in our stream here:

        task.row_size = int(cnfgs.appconfigs().configs().db_cols());


        threads.emplace_back([&]() {
            grpc::ChannelArguments ch_args;
            ch_args.SetMaxReceiveMessageSize(constants::max_message_size);
            ch_args.SetMaxSendMessageSize(constants::max_message_size);
            this->stub = distribicom::Manager::NewStub(
                    grpc::CreateCustomChannel(
                            cnfgs.appconfigs().main_server_hostname(),
                            grpc::InsecureChannelCredentials(),
                            ch_args
                    )
            );

            distribicom::WorkerRegistryRequest rqst;
            this->stub->async()->RegisterAsWorker(&context_, &rqst, this);

            StartRead(&read_val); // queueing a read request.
            StartCall();

            wait_for_stream_termination();
        });
    }


    grpc::Status Worker::wait_for_stream_termination() {
        std::unique_lock<std::mutex> l(mu_);
        cv_.wait(l, [this] { return done_; });
        return std::move(status_);
    }

    void Worker::OnDone(const grpc::Status &s) {
        std::unique_lock<std::mutex> l(mu_);
        status_ = s;
        done_ = true;
        cv_.notify_one();
    }
}

