#include "worker.hpp"

namespace services {
    void add_matrix_size(grpc::ClientContext &context, int size) {
        context.AddMetadata(MD_MATRIX_SIZE, std::to_string(size));
    }

    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp) {
        auto ntmp = mp.find(MD_MATRIX_SIZE);
        try {
            return std::stoi(std::string(ntmp->second.data(), ntmp->second.size()));
        } catch (...) {
            return -1;
        }
    }

    Worker::Worker(distribicom::AppConfigs &_app_configs)
            : symmetric_secret_key(), app_configs(_app_configs), chan(), mrshl() {
        if (app_configs.configs().scheme() != "bgv") {
            throw std::invalid_argument("currently not supporting any scheme oother than bgv.");
        }

        seal::EncryptionParameters enc_params(seal::scheme_type::bgv);
        auto N = app_configs.configs().polynomial_degree();
        auto logt = app_configs.configs().logarithm_plaintext_coefficient();

        enc_params.set_poly_modulus_degree(N);
        // TODO: Use correct BGV parameters.
        enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
        enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));

        mrshl = marshal::Marshaller::Create(enc_params);
    }

    grpc::Status
    Worker::SendTasks(grpc::ServerContext *context, grpc::ServerReader<::distribicom::MatrixPart> *reader,
                      distribicom::Ack *response) {
        response->set_success(false);
        try {
            WorkerServiceTask task(context);

            distribicom::MatrixPart tmp;
            while (reader->Read(&tmp)) {
                fill_task(task, tmp);
            }

            for (int i = 0; i < 5; ++i) {
                std::cout << task.ptx_rows[0][i].to_string() << std::endl;
            }

            response->set_success(true);
            // TODO: Continue once we've stopped receiving.
            std::cout << "Disconnecting" << std::endl;
        } catch (std::invalid_argument &e) {
            std::cout << "Exception: " << e.what() << std::endl;
            return {grpc::StatusCode::INVALID_ARGUMENT, e.what()};
        }

        return {};
    }

    void Worker::fill_task(WorkerServiceTask &task, const distribicom::MatrixPart &tmp) const {
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
            task.ctx_cols[col] = std::vector<seal::Ciphertext>(task.row_size);
        }
        if (row >= task.row_size) {
            throw std::invalid_argument("Invalid row index, too big");
        }
        task.ctx_cols[col][row] = mrshl->unmarshal_seal_object<seal::Ciphertext>(tmp.ptx().data());
    }

    WorkerServiceTask::WorkerServiceTask(grpc::ServerContext *pContext) :
            epoch(-1), round(-1), row_size(-1), ptx_rows(), ctx_cols() {
        const auto &metadata = pContext->client_metadata();
        row_size = extract_size_from_metadata(metadata);
        if (row_size <= 0) {
            throw std::invalid_argument("row size must be positive");
        }
        if (row_size > 10 * 1000 || row_size <= 0) {
            throw std::invalid_argument("row size must be between 0 and 10k");
        }
        if (epoch < 0) {
            throw std::invalid_argument("epoch must be positive");
        }
        if (round < 0) {
            throw std::invalid_argument("round must be positive");
        }
    }
}

