#include "worker.hpp"
#include "utils.hpp"
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
        manager_conn = std::make_unique<distribicom::Manager::Stub>(distribicom::Manager::Stub(
                grpc::CreateChannel(
                        cnfgs.appconfigs().main_server_hostname(),
                        grpc::InsecureChannelCredentials()
                )
        ));

        grpc::ClientContext context;
        distribicom::Ack response;
        distribicom::WorkerRegistryRequest request;

        request.set_workerport(cnfgs.workerport());

        manager_conn->RegisterAsWorker(&context, request, &response);
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
                     distribicom::Ack *response) {
        try {
            WorkerServiceTask task(context, cnfgs.appconfigs().configs());

            distribicom::WorkerTaskPart tmp;
            while (reader->Read(&tmp)) {
                if (!tmp.has_matrixpart()) {
                    // TODO update the worker's galois keys perhaps?
                    throw NotImplemented("receiving GaloisKey not implemented yet.");
                }
                fill_matrix_part(task, tmp.matrixpart());
            }
            chan.write(task);
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
}

