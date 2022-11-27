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
        std::future<void>
        async_expand_query(int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &&qry) {
            // TODO: ensure this utilises a threadpool. otherwise this isn't great.
            return std::async(
                    [&](int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &&qry) {
                        mu.lock_shared();
                        auto not_exist = gkeys.find(query_pos) == gkeys.end();
                        mu.unlock_shared();

                        if (not_exist) {
                            throw std::runtime_error(
                                    "galois keys not found for query position: " + std::to_string(query_pos));
                        }

                        auto expanded = query_expander->expand_query(qry, expanded_size, gkeys.find(query_pos)->second);

                        mu.lock();
                        queries.insert({query_pos, math_utils::matrix<seal::Ciphertext>(expanded.size(), 1, expanded)});
                        mu.unlock();
                    },
                    query_pos, expanded_size, qry
            );
        }

    protected:
        // TODO: make a unique ptr!
        std::shared_ptr<math_utils::QueryExpander> query_expander;
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::map<int, math_utils::matrix<seal::Ciphertext> > queries;
        std::map<int, seal::GaloisKeys> gkeys;
        std::shared_mutex mu;


        void expand_queries(WorkerServiceTask &task) {
            std::vector<std::future<void>> futures(task.ctx_cols.size());

            for (auto &pair: task.ctx_cols) {
                futures.push_back(async_expand_query(pair.first, task.row_size, std::move(pair.second)));
            }

            for (auto &future: futures) {
                future.wait();
            }
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
        std::unique_ptr<distribicom::Manager::Stub> manager_conn;
    public:
        explicit RowMultiplicationStrategy(const seal::EncryptionParameters &enc_params,
                                           std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) :
                WorkerStrategy(enc_params), manager_conn(std::move(manager_conn)) {};

        void process_task(WorkerServiceTask &&task) override {
            // expand all queries.
            expand_queries(task);

        }
    };
}