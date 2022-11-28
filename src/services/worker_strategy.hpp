#pragma once

#include <future>
#include "distribicom.grpc.pb.h"
#include "math_utils/matrix_operations.hpp"
#include "math_utils/query_expander.hpp"
// TODO: reconsider class name

namespace services {

    struct WorkerServiceTask {
        explicit WorkerServiceTask() : epoch(-1), round(-1), row_size(-1) {};

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

    private:
        std::future<int>
        async_expand_query(int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &&qry);

    protected:
        // TODO: make a unique ptr!
        std::shared_ptr<math_utils::QueryExpander> query_expander;
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::map<int, math_utils::matrix<seal::Ciphertext> > queries;
        std::map<int, seal::GaloisKeys> gkeys;
        std::shared_mutex mu;
        std::unique_ptr<distribicom::Manager::Stub> manager_conn;


        void expand_queries(WorkerServiceTask &task);

    public:
        explicit WorkerStrategy(const seal::EncryptionParameters &enc_params,
                                std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) noexcept;

        void store_galois_key(const seal::GaloisKeys &keys, int query_position) {
            gkeys[query_position] = keys;
        };

        virtual ~WorkerStrategy() = default;

        virtual void process_task(WorkerServiceTask &&task) = 0;

    };

    class RowMultiplicationStrategy : public WorkerStrategy {
    public:
        explicit RowMultiplicationStrategy(const seal::EncryptionParameters &enc_params,
                                           std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) :
                WorkerStrategy(enc_params, std::move(manager_conn)) {};

        void process_task(WorkerServiceTask &&task) override {
            // expand all queries.
            expand_queries(task);

        }
    };
}