#pragma once

#include <future>
#include "distribicom.grpc.pb.h"
#include "math_utils/matrix_operations.hpp"
#include "math_utils/query_expander.hpp"
#include "marshal/marshal.hpp"

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


    protected:
        // TODO: make a unique ptr!
        std::shared_ptr<math_utils::QueryExpander> query_expander;
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::map<int, seal::GaloisKeys> gkeys;
        std::shared_mutex mu;
        std::unique_ptr<distribicom::Manager::Stub> manager_conn;

    public:
        explicit WorkerStrategy(const seal::EncryptionParameters &enc_params,
                                std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) noexcept;

        void store_galois_key(seal::GaloisKeys &&keys, int query_position) {
            mu.lock();
            gkeys[query_position] = std::move(keys);
            mu.unlock();
        };

        virtual ~WorkerStrategy() = default;

        virtual void process_task(WorkerServiceTask &&task) = 0;

    };

    class RowMultiplicationStrategy : public WorkerStrategy {
        std::map<int, math_utils::matrix<seal::Ciphertext> > queries;
        math_utils::matrix<seal::Ciphertext> query_mat;
        std::map<int, int> col_to_query_index;
        std::shared_ptr<marshal::Marshaller> mrshl;

        std::future<int>
        async_expand_query(int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &&qry);


        void expand_queries(WorkerServiceTask &task);

    public:
        explicit RowMultiplicationStrategy(const seal::EncryptionParameters &enc_params,
                                           std::shared_ptr<marshal::Marshaller> &m,
                                           std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) :
                WorkerStrategy(enc_params, std::move(manager_conn)), mrshl(m) {
        };

        math_utils::matrix<seal::Ciphertext> multiply_rows(WorkerServiceTask &task);

        void send_response(const WorkerServiceTask &task,
                           math_utils::matrix<seal::Ciphertext> &computed);

// assumes there is data to process in the task.
        void process_task(WorkerServiceTask &&task) override {
            // expand all queries.
            expand_queries(task);
            if(task.ptx_rows.empty()){ return;}
            auto answer = multiply_rows(task);

            send_response(task, answer);
        }

        void queries_to_mat(const WorkerServiceTask &task);
    };
}