#pragma once

#include "distribicom.grpc.pb.h"
#include "seal/seal.h"
#include "constants.hpp"
#include <string>

#include "concurrency/channel.hpp"
#include "marshal/marshal.hpp"
#include "math_utils/query_expander.hpp"

namespace services {

    struct WorkerServiceTask {
        explicit WorkerServiceTask(grpc::ServerContext *pContext, const distribicom::Configs &configs);

        // metadata worker needs to know.
        int epoch;
        int round;

        // matrices:
        int row_size;
        std::map<int, std::vector<seal::Plaintext>> ptx_rows;
        std::map<int, std::vector<seal::Ciphertext>> ctx_cols;
    };

    class Worker final : public distribicom::Worker::Service {
        // used by the worker to ensure it receives authentic data.
        std::vector<std::byte> symmetric_secret_key;

        // states how this worker will operate.
        distribicom::WorkerConfigs cnfgs;

        concurrency::Channel<WorkerServiceTask> chan;
        std::shared_ptr<marshal::Marshaller> mrshl;
        std::shared_ptr<math_utils::QueryExpander> query_expander;
        std::unique_ptr<distribicom::Manager::Stub> manager_conn;

    public:

        explicit Worker(distribicom::WorkerConfigs &&wcnfgs);

        grpc::Status
        SendTask(grpc::ServerContext *context, grpc::ServerReader<::distribicom::WorkerTaskPart> *reader,
                 distribicom::Ack *response) override;

        void fill_matrix_part(WorkerServiceTask &task, const distribicom::MatrixPart &tmp) const;

        void inspect_configs() const;

        seal::EncryptionParameters setup_enc_params() const;
    };
}
