#include "worker_strategy.hpp"
#include "utils.hpp"

namespace services::work_strategy {
    WorkerStrategy::WorkerStrategy(const seal::EncryptionParameters &enc_params,
                                   std::unique_ptr<distribicom::Manager::Stub> &&manager_conn) noexcept
        : gkeys(), manager_conn(std::move(manager_conn)) {

        pool = std::make_shared<concurrency::threadpool>();

        query_expander = math_utils::QueryExpander::Create(
            enc_params,
            pool
        );

        matops = math_utils::MatrixOperations::Create(
            math_utils::EvaluatorWrapper::Create(enc_params),
            pool
        );
    }

    // responsible for caching the queries inside a matrix.
    void RowMultiplicationStrategy::queries_to_mat(const WorkerServiceTask &task) {
        query_mat.resize(task.row_size, queries.size());
        auto col = -1;
        for (auto &pair: queries) {
            col += 1;
            auto full_query_column = pair.second;

            for (std::uint64_t row = 0; row < full_query_column.rows; ++row) {
                query_mat(row, col) = std::move(full_query_column(row, 0));
            }
        }
        matops->to_ntt(query_mat.data);
    }

    void RowMultiplicationStrategy::expand_queries(WorkerServiceTask &task) {
        if (task.ctx_cols.empty()) {
            return;
        }

        std::cout<<"doing expansions for:"<< task.ctx_cols.size()<<std::endl;
        auto promises = async_expand_promises(task);

        mu.lock();
        for (auto &pair: promises) {
            auto expanded = *(pair.second->get());
            queries.insert(
                {
                    pair.first,
                    math_utils::matrix<seal::Ciphertext>(expanded.size(), 1, expanded)
                }
            );
        }
        mu.unlock();

        queries_to_mat(task);

        auto col = -1;
        for (auto &key_val: queries) {
            col_to_query_index[++col] = key_val.first;
        }

    }

    std::vector<std::pair<int, std::shared_ptr<concurrency::promise<std::vector<seal::Ciphertext>>>>>
    RowMultiplicationStrategy::async_expand_promises(WorkerServiceTask &task) {
        std::vector<std::pair<int, std::shared_ptr<concurrency::promise<std::vector<seal::Ciphertext>>>>> promises;
        promises.reserve(task.ctx_cols.size());

        std::shared_lock lock(mu);
        for (auto &pair: task.ctx_cols) {
            auto query_pos = pair.first;
            auto expanded_size = task.row_size;

            if (gkeys.find(query_pos) == gkeys.end()) {
                throw std::runtime_error("galois keys not found for query position " + std::to_string(query_pos));
            }

            promises.emplace_back(
                query_pos,
                query_expander->async_expand(
                    std::move(pair.second),
                    expanded_size,
                    gkeys.find(query_pos)->second
                )
            );
        }

        return promises;
    }

    math_utils::matrix<seal::Ciphertext> RowMultiplicationStrategy::multiply_rows(WorkerServiceTask &task) {
        // i have queries, which are columns to multiply with the rows.
        // i set the task's ptxs in a matrix of the correct size.
        auto mat_size = int(task.ptx_rows.size());
        math_utils::matrix<seal::Plaintext> ptxs(mat_size, task.row_size);

        // moving from a map of rows to a matrix.
        auto row = -1;
        for (auto &pair: task.ptx_rows) {
            row += 1;
            auto ptx_row = pair.second;
            for (std::uint64_t col = 0; col < ptx_row.size(); ++col) {
                ptxs(row, col) = std::move(ptx_row[col]);
            }
        }

        math_utils::matrix<seal::Ciphertext> result;
        matops->multiply(ptxs, query_mat, result);

        return result;
    }

    void
    RowMultiplicationStrategy::send_response(
        const WorkerServiceTask &task,
        math_utils::matrix<seal::Ciphertext> &computed
    ) {
        std::map<int, int> row_to_index;
        auto row = -1;
        for (auto &p: task.ptx_rows) {
            row_to_index[++row] = p.first;
        }

        grpc::ClientContext context;
        utils::add_metadata_string(context, constants::credentials_md, sym_key);
        utils::add_metadata_size(context, constants::round_md, task.round);
        utils::add_metadata_size(context, constants::epoch_md, task.epoch);

        // before sending the response - ensure we send it in reg-form to compress the data a bit more.
        matops->from_ntt(computed.data);

        distribicom::Ack resp;
        auto stream = manager_conn->ReturnLocalWork(&context, &resp);

        distribicom::MatrixPart tmp;
        for (std::uint64_t i = 0; i < computed.rows; ++i) {
            for (std::uint64_t j = 0; j < computed.cols; ++j) {
                tmp.mutable_ctx()->set_data(mrshl->marshal_seal_object(computed(i, j)));
                tmp.set_row(row_to_index[int(i)]);
                tmp.set_col(col_to_query_index[int(j)]);
                stream->Write(tmp);
            }
        }

        stream->WritesDone();
        auto status = stream->Finish();
        if (!status.ok()) {
            std::cout << "Services::RowMultiplicationStrategy:: hadn't completed its stream:" + status.error_message()
                      << std::endl;
        }
    }
}
