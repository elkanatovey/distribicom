#include "worker_strategy.hpp"

namespace services::work_strategy {
    WorkerStrategy::WorkerStrategy(const seal::EncryptionParameters &enc_params,
                                   std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) noexcept
            :
            query_expander(), matops(), queries(), gkeys(), manager_conn(std::move(manager_conn)) {
        query_expander = math_utils::QueryExpander::Create(enc_params);
        matops = math_utils::MatrixOperations::Create(
                math_utils::EvaluatorWrapper::Create(enc_params)
        );
    }

    void WorkerStrategy::expand_queries(WorkerServiceTask &task) {
        std::vector<std::future<int>> futures;
        futures.reserve(task.ctx_cols.size());

        for (auto &pair: task.ctx_cols) {
            futures.push_back(async_expand_query(pair.first, task.row_size, std::move(pair.second)));
        }

        std::for_each(
                futures.begin(),
                futures.end(),
                [](std::future<int> &future) { future.get(); }
        );
    }

    std::future<int>
    WorkerStrategy::async_expand_query(int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &&qry) {
        // TODO: ensure this utilises a threadpool. otherwise this isn't great.
        return std::async(
                [&](int query_pos, int expanded_size, const std::vector<seal::Ciphertext> &&qry) {
                    mu.lock_shared();
                    auto not_exist = gkeys.find(query_pos) == gkeys.end();
                    mu.unlock_shared();

                    if (not_exist) {
                        std::cout << "galois keys not found for query position:" + std::to_string(query_pos)
                                  << std::endl;
                        return 0;
                    }

                    auto expanded = query_expander->expand_query(qry, expanded_size, gkeys.find(query_pos)->second);

                    mu.lock();
                    queries.insert({query_pos, math_utils::matrix<seal::Ciphertext>(expanded.size(), 1, expanded)});
                    mu.unlock();
                    return 1;
                },
                query_pos, expanded_size, qry
        );
    }
}
