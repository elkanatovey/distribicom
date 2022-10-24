#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "seal/seal.h"
#include "constants.hpp"
#include <string>

// TODO: Ask Elkana how to import correctly.
#include "../math_utils/channel.h"
#include "../marshal/marshal.hpp"
#include "../math_utils/query_expander.hpp"

namespace services {
    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                   const constants::metadata &md);

    void add_metadata_size(grpc::ClientContext &context, const constants::metadata &md, int size);

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
        distribicom::AppConfigs app_configs;

        Channel<WorkerServiceTask> chan;
        std::shared_ptr<marshal::Marshaller> mrshl;
        std::shared_ptr<multiplication_utils::QueryExpander> query_expander;

    public:

        explicit Worker(distribicom::AppConfigs &app_configs);

        grpc::Status
        SendTask(grpc::ServerContext *context, grpc::ServerReader<::distribicom::WorkerTaskPart> *reader,
                  distribicom::Ack *response) override;

        void fill_matrix_part(WorkerServiceTask &task, const distribicom::MatrixPart &tmp) const;

        void inspect_configs() const;

        seal::EncryptionParameters setup_enc_params() const;
    };
}
