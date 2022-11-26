#pragma once

#include "distribicom.grpc.pb.h"
#include "math_utils/matrix_operations.hpp"
#include "math_utils/query_expander.hpp"
// TODO: reconsider class name

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
}

namespace services::work_strategy {

    /**
     * A worker strategy is a class that defines how a worker will operate in a given round.
     * Upon constracting a WorkerServiceTask it'll forward the work to the strategy to conclude operations.
     * Once the strategy is done processing the task, it'll return a result to the worker.
     */
    class WorkerStrategy {
    protected:
        // TODO: make a unique ptr!
        // TODO: instead of queryExpander, maybe something else should be responsible? im not certain yet.
        std::shared_ptr<math_utils::QueryExpander> query_expander;
        std::shared_ptr<math_utils::MatrixOperations> matops;
        math_utils::matrix<seal::Ciphertext> queries;

        // TODO: expand and store the full queries in  queries
        // TODO: should be perform in an async manner.
        void expand_incoming_queries(const std::map<int, std::vector<seal::Ciphertext>> &ctx_cols) {};

    public:
        explicit WorkerStrategy(const seal::EncryptionParameters &enc_params) noexcept:
                query_expander(), matops(), queries() {
            query_expander = math_utils::QueryExpander::Create(enc_params);
            matops = math_utils::MatrixOperations::Create(
                    math_utils::EvaluatorWrapper::Create(enc_params)
            );
        }

        void store_galois_key(const seal::GaloisKeys &keys, std::uint64_t query_position) {};

        virtual ~WorkerStrategy() = default;

        virtual void process_task(WorkerServiceTask task) = 0;
    };

    class RowMultiplicationStrategy : public WorkerStrategy {
    };
}