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

        auto task = WorkerServiceTask{
                0, // todo: extract from metadata.
                0, // todo: extract from metadata.
                extract_size_from_metadata(context->client_metadata()),
                std::map<int, std::vector<seal::Plaintext>>(),
                std::map<int, std::vector<seal::Ciphertext>>(),
        };

        try {
            vaildate_task(task);
        } catch (std::invalid_argument &e) {
            return {grpc::StatusCode::INVALID_ARGUMENT, e.what()};
        }


        distribicom::MatrixPart tmp;
        distribicom::Plaintext tmp_ptx;
        distribicom::Ciphertext tmp_ctx;
        int row, col;
        while (reader->Read(&tmp)) {
            row = tmp.row();
            col = tmp.col();

            if (tmp.has_ptx()) {
                if (!task.ptx_rows.contains(row)) {
                    task.ptx_rows[row] = std::vector<seal::Plaintext>(task.row_size);
                }
                if (col >= task.row_size) {
                    return {grpc::StatusCode::INVALID_ARGUMENT, "col is too big"};
                }
                task.ptx_rows[row][col]
                        = mrshl->unmarshal_seal_object<seal::Plaintext>(tmp.ptx().data());
                continue;
            }

            if (!tmp.has_ctx()) {
                continue;
            }
            // assumes either a ptx or a ctx.
            if (!task.ctx_cols.contains(col)) {
                task.ctx_cols[col] = std::vector<seal::Ciphertext>(task.row_size);
            }
            if (row >= task.row_size) {
                return {grpc::StatusCode::INVALID_ARGUMENT, "row is too big"};
            }
            task.ctx_cols[col][row]
                    = mrshl->unmarshal_seal_object<seal::Ciphertext>(tmp.ptx().data());
        }

        response->set_success(true);
        // TODO: Continue once we've stopped receiving.
        std::cout << "Disconnecting" << std::endl;
        return {};
    }

    void Worker::vaildate_task(WorkerServiceTask &task) const {
        if (task.row_size <= 0) {
            throw std::invalid_argument("row size must be positive");
        }
        if (task.row_size > 10 * 1000 || task.row_size <= 0) {
            throw std::invalid_argument("row size must be between 0 and 10k");
        }
        if (task.epoch < 0) {
            throw std::invalid_argument("epoch must be positive");
        }
        if (task.round < 0) {
            throw std::invalid_argument("round must be positive");
        }
    }

}

