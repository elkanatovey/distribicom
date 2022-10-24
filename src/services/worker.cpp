#include "worker.hpp"

namespace services {
    void add_metadata_size(grpc::ClientContext &context, const constants::metadata &md, int size) {
        context.AddMetadata(std::string(md), std::to_string(size));
    }

    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                   const constants::metadata &md) {
        auto ntmp = mp.find(std::string(md));
        return std::stoi(std::string(ntmp->second.data(), ntmp->second.size()));
    }

    Worker::Worker(distribicom::AppConfigs &_app_configs)
            : symmetric_secret_key(), app_configs(_app_configs), chan(), mrshl() {
        if (app_configs.configs().scheme() != "bgv") {
            throw std::invalid_argument("currently not supporting any scheme oother than bgv.");
        }
        auto row_size = app_configs.configs().db_cols();
        if (row_size <= 0) {
            throw std::invalid_argument("row size must be positive");
        }
        if (row_size > 10 * 1000 || row_size <= 0) {
            throw std::invalid_argument("row size must be between 1 and 10k");
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
            WorkerServiceTask task(context, app_configs.configs());

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
            task.ctx_cols[col] = std::vector<seal::Ciphertext>(1);
        }
        if (row >= 1) {
            throw std::invalid_argument("should receive at most one compressed ciphertext");
        }
        task.ctx_cols[col][row] = mrshl->unmarshal_seal_object<seal::Ciphertext>(tmp.ptx().data());
    }

    WorkerServiceTask::WorkerServiceTask(grpc::ServerContext *pContext, const distribicom::Configs &configs) :
            epoch(-1), round(-1), row_size(int(configs.db_cols())), ptx_rows(), ctx_cols() {

        const auto &metadata = pContext->client_metadata();
        epoch = extract_size_from_metadata(metadata, constants::epoch_md);
        round = extract_size_from_metadata(metadata, constants::round_md);

        if (epoch < 0) {
            throw std::invalid_argument("epoch must be positive");
        }
        if (round < 0) {
            throw std::invalid_argument("round must be positive");
        }
    }
}

