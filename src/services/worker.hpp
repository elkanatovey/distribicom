#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "../math_utils/channel.h" // TODO: Ask Elkana how to import correctly.
#include "../marshal/marshal.hpp" // TODO: Ask Elkana how to import correctly.
#include "seal/seal.h"
#include <string>

#define MD_MATRIX_SIZE "m_size"
namespace services {
    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp);

    void add_matrix_size(grpc::ClientContext &context, int size);

    // todo this is what the worker ends the SendTasks RPC call with.
    //  should contain things the worker ought to send to it's threadpool.
    struct WorkerTask {
    };

    class Worker final : public distribicom::Worker::Service {
        // used by the worker to ensure it receives authentic data.
        std::vector<std::byte> symmetric_secret_key;

        // states how this worker will operate.
        distribicom::AppConfigs app_configs;

        Channel<WorkerTask> chan;
        std::shared_ptr<marshal::Marshaller> mrshl;
    public:
        // register to the server.
        void start() {};

        explicit Worker(distribicom::AppConfigs &app_configs);

        grpc::Status
        SendTasks(grpc::ServerContext *context, grpc::ServerReader<::distribicom::MatrixPart> *reader,
                  distribicom::Ack *response) override;

    };
}
