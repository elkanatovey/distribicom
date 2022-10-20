#include "worker.hpp"

namespace services {
    static int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp) {
        auto ntmp = mp.find(MD_MATRIX_SIZE);
        try {
            return std::stoi(std::string(ntmp->second.data(), ntmp->second.size()));
        } catch (...) {
            return -1;
        }
    }

    static void add_matrix_size(grpc::ClientContext &context, int size) {
        context.AddMetadata(MD_MATRIX_SIZE, std::to_string(size));
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

        auto n = extract_size_from_metadata(context->client_metadata());
        if (n > 10 * 1000 || n <= 0) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "bad matrix size received in metadata"};
        }
        std::cout << "GOT: " << n << std::endl;

        std::map<int, std::vector<seal::Plaintext>> ptx_rows;
        std::map<int, std::vector<seal::Ciphertext>> ctx_cols;

        distribicom::MatrixPart tmp;
        distribicom::Plaintext tmp_ptx;
        distribicom::Ciphertext tmp_ctx;
        int row, col;
        while (reader->Read(&tmp)) {
            row = tmp.row();
            col = tmp.col();

            if (tmp.has_ptx()) {
                if (!ptx_rows.contains(row)) {
                    ptx_rows[row] = std::vector<seal::Plaintext>(n);
                }
                ptx_rows[row][col]
                        = mrshl->unmarshal_seal_object<seal::Plaintext>(tmp.ptx().data());
                continue;
            }

            if (!tmp.has_ctx()) {
                continue;
            }
            // assumes either a ptx or a ctx.
            if (!ctx_cols.contains(col)) {
                ctx_cols[col] = std::vector<seal::Ciphertext>(n);
            }
            ctx_cols[col][row]
                    = mrshl->unmarshal_seal_object<seal::Ciphertext>(tmp.ptx().data());
        }
        response->set_success(true);
        // TODO: Continue once we've stopped receiving.
        std::cout << "Disconnecting" << std::endl;
        return {};
    }

}

