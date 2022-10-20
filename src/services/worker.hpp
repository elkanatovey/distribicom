#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "boot.pb.h"
#include "../math_utils/channel.h" // TODO: Ask Elkana how to import correctly.
#include "../marshal/marshal.hpp" // TODO: Ask Elkana how to import correctly.
#include "seal/seal.h"
#include <string>

#define MD_MATRIX_SIZE "m_size"
namespace services {

    // TODO: put in some other file the following two functions
    static int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp) {
        auto ntmp = mp.find(MD_MATRIX_SIZE);
        try {
            return std::stoi(std::string(ntmp->second.data(), ntmp->second.size()));
        } catch (...) {
            return -1;
        }
    }

    static int add_matrix_size(grpc::ClientContext &context, int size) {
        context.AddMetadata(MD_MATRIX_SIZE, std::to_string(size));
    }


    struct WorkerTask {
        // todo this is what the worker ends the SendTasks RPC call with.
    };

    class Worker final : public distribicom::Worker::Service {
        // used by the worker to ensure it receives authentic data.
        std::vector<std::byte> symmetric_secret_key;

        // states how this worker will operate.
//        distribicom_boot::DistribicomConfigs configs;

        Channel<WorkerTask> chan;
        std::unique_ptr<marshal::Marshaller> mrshl;
    public:
        // register to the server.
        void start() {};

        Worker() : symmetric_secret_key(), chan(), mrshl() {
            // todo: init mrshl.
        };

        grpc::Status
        SendTasks(grpc::ServerContext *context, grpc::ServerReader<::distribicom::MatrixPart> *reader,
                  distribicom::Ack *response) override {
            auto n = extract_size_from_metadata(context->client_metadata());
            if (n > 10 * 1000 || n <= 0) {
                return {grpc::StatusCode::INVALID_ARGUMENT, "bad matrix size received in metadata"};
            }
            std::cout << "GOT: " << n << std::endl;

            std::map<int, std::vector<seal::Plaintext>> ptx_rows;
            std::map<int, std::vector<seal::Ciphertext>> ctx_cols;

            distribicom::MatrixPart tmp;
            distribicom::Plaintext tmp_ptx;
            while (reader->Read(&tmp)) {
                if (tmp.has_ptx()) {
                    if (!ptx_rows.contains(tmp.row())) {
                        ptx_rows[tmp.row()] = std::vector<seal::Plaintext>(n);
                    }
//                    ptx_rows[tmp.row()].push_back()
//                            = mrshl->unmarshal_seal_object<seal::Plaintext>(tmp.ptx().data());
                    // todo:
                    continue;
                }

                if (tmp.has_ctx()) {

                }
            }

            std::cout << "Disconnecting" << std::endl;
            return {};
        }

    };
}
