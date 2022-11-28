#pragma once

#include "distribicom.grpc.pb.h"
#include "seal/seal.h"
#include "constants.hpp"
#include <string>

#include "concurrency/channel.hpp"
#include "marshal/marshal.hpp"
#include "math_utils/query_expander.hpp"
#include "worker_strategy.hpp"

namespace services {


    class Worker final : public distribicom::Worker::Service {
        // used by the worker to ensure it receives authentic data.
        std::vector<std::byte> symmetric_secret_key;

        // states how this worker will operate.
        distribicom::WorkerConfigs cnfgs;

        concurrency::Channel<WorkerServiceTask> chan;
        std::shared_ptr<marshal::Marshaller> mrshl;
        std::unique_ptr<std::thread> t;

    public:
        // making this public for now, so that we can test it.
        std::shared_ptr<work_strategy::RowMultiplicationStrategy> strategy;


        explicit Worker(distribicom::WorkerConfigs &&wcnfgs);

        grpc::Status
        SendTask(grpc::ServerContext *context, grpc::ServerReader<::distribicom::WorkerTaskPart> *reader,
                 distribicom::Ack *response) override;

        void fill_matrix_part(WorkerServiceTask &task, const distribicom::MatrixPart &tmp) const;

        void inspect_configs() const;

        void close();
    };
}
