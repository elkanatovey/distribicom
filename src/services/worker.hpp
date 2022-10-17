#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "boot.pb.h"
#include "../math_utils/channel.h" // TODO: Ask Elkana how to import correctly.
#include "../marshal/marshal.hpp" // TODO: Ask Elkana how to import correctly.
#include "seal/seal.h"

namespace services {
    struct WorkerTask {
        // todo this is what the worker ends the SendTasks RPC call with.
    };

    class Worker final : public distribicom::Worker::Service {
        // used by the worker to ensure it receives authentic data.
        std::vector<std::byte> symmetric_secret_key;

        // states how this worker will operate.
        distribicom_boot::DistribicomConfigs configs;

        Channel<WorkerTask> chan;
        std::unique_ptr<marshal::Marshaller> mrshl;
    public:
        // register to the server.
        void start() {};

        Worker() : symmetric_secret_key(), configs(), chan(), mrshl() {
            // todo: init mrshl.
        };

        ::grpc::Status
        SendTasks(::grpc::ServerContext *context, ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                  ::distribicom::Ack *response) override {
            auto mt = context->client_metadata(); // todo: not certain yet on how to use this.


            std::map<int, std::vector<seal::Plaintext>> ptx_rows;
            std::map<int, std::vector<seal::Ciphertext>> ctx_cols;

            distribicom::MatrixPart tmp;
            distribicom::Plaintext tmp_ptx;
            while (reader->Read(&tmp)) {
                if (tmp.has_ptx()) {
                    mrshl->unmarshal_seal_object<seal::Plaintext>(tmp.ptx().data());
                    ptx_rows[tmp.row()] =
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
