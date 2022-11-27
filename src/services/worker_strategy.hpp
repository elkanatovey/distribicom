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
        // TODO: the current item represents one dimension only!
        //      to expand it we might need other parameters to specify which dimension (for example left side query).
        std::map<int, std::vector<seal::Ciphertext>> ctx_cols;
    };
}

namespace services::work_strategy {
    enum type {
        frievalds,
        fft,
    };

    /**
     * A worker strategy is a class that defines how a worker will operate in a given round.
     * Upon constracting a WorkerServiceTask it'll forward the work to the strategy to conclude operations.
     * Once the strategy is done processing the task, it'll return a result to the worker.
     */
    class WorkerStrategy {

    protected:
        // TODO: make a unique ptr!
        std::shared_ptr<math_utils::QueryExpander> query_expander;
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::map<int, math_utils::matrix<seal::Ciphertext> > queries;
        std::map<int, seal::GaloisKeys> gkeys;

        void expand_query(int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &qry) {
            if (gkeys.find(query_pos) == gkeys.end()) {
                throw std::runtime_error("galois keys not found for query position: " + std::to_string(query_pos));
            }

            auto expanded = query_expander->expand_query(qry, expanded_size, gkeys.find(query_pos)->second);
            queries.insert({query_pos, math_utils::matrix<seal::Ciphertext>(expanded.size(), 1, expanded)});
        }

    public:
        explicit WorkerStrategy(const seal::EncryptionParameters &enc_params) noexcept:
                query_expander(), matops(), queries(), gkeys() {
            query_expander = math_utils::QueryExpander::Create(enc_params);
            matops = math_utils::MatrixOperations::Create(
                    math_utils::EvaluatorWrapper::Create(enc_params)
            );
        }

        void store_galois_key(const seal::GaloisKeys &keys, int query_position) {
            gkeys[query_position] = keys;
        };

        virtual ~WorkerStrategy() = default;

        virtual void process_task(WorkerServiceTask &&task) = 0;

    };

    class RowMultiplicationStrategy : public WorkerStrategy {
        void process_task(WorkerServiceTask &&task) override {
            // expand all queries.

            // then perform mat mul.

            // then push responses back.
        }
    };
}