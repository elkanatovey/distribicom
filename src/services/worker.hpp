#pragma once

#include "distribicom.grpc.pb.h"
#include "seal/seal.h"
#include "constants.hpp"
#include <string>

#include "concurrency/channel.hpp"
#include "marshal/marshal.hpp"
#include "math_utils/query_expander.hpp"
#include "worker_strategy.hpp"
#include "distribicom.pb.h"

namespace services {


    /**
     * Worker is a client-reactor. upon boot will call the server and and start receiving incoming streams from it.
     */
    class Worker final : public grpc::ClientReadReactor<distribicom::WorkerTaskPart> {
        // used by the worker to ensure it receives authentic data.
        std::vector<std::byte> symmetric_secret_key;

        // states how this worker will operate.
        distribicom::WorkerConfigs cnfgs;

        concurrency::Channel<WorkerServiceTask> chan;
        std::shared_ptr<marshal::Marshaller> mrshl;
        std::vector<std::thread> threads;
        std::unique_ptr<::grpc::ClientReader<::distribicom::WorkerTaskPart>> handle;


        // Stream-Reader Properties:
        WorkerServiceTask task;
        std::unique_ptr<distribicom::Manager::Stub> stub;
        grpc::ClientContext context_;
        distribicom::WorkerTaskPart read_val;
        std::mutex mu_;
        std::condition_variable cv_;
        grpc::Status status_;
        bool done_ = false;

        void setup_stream();

        // The following are calls done by gRPC.
        void OnDone(const grpc::Status &s) override;

        grpc::Status wait_for_stream_termination();

        void OnReadDone(bool ok) override;

        void inspect_configs() const;

        void update_current_task();

    public:
        // making this public for now, so that we can test it.
        std::shared_ptr<work_strategy::RowMultiplicationStrategy> strategy;


        explicit Worker(distribicom::WorkerConfigs &&wcnfgs);


        void close();

    };
}
