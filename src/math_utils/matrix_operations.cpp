#include "matrix_operations.hpp"
#include "defines.h"
#include <execution>
#include <latch>

namespace math_utils {
    // helper funcs:

    template<typename T, typename U>
    void verify_not_empty_matrices(const matrix<T> &left, const matrix<U> &right) {
        if (left.data.empty()) {
            throw std::invalid_argument("MatrixOperations::multiply: received empty left matrix");
        }
        if (right.data.empty()) {
            throw std::invalid_argument("MatrixOperations::multiply: received empty right matrix");
        }
    }

    void verify_ptx_ctx_mat_mul_args(const matrix<seal::Plaintext> &left, const matrix<seal::Ciphertext> &right) {
        verify_not_empty_matrices(left, right);
        if (left.data[0].is_ntt_form()) {
            throw std::invalid_argument("MatrixOperations::multiply: left matrix should not be in NTT form");
        }
        if (!right.data[0].is_ntt_form()) {
            throw std::invalid_argument("MatrixOperations::multiply: right matrix should be in NTT form");
        }
    }

    void
    verify_spltptx_ctx_mat_mul_args(const matrix<SplitPlaintextNTTForm> &left, const matrix<seal::Ciphertext> &right) {
        verify_not_empty_matrices(left, right);
        if (!left.data[0][0].is_ntt_form()) {
            throw std::invalid_argument("MatrixOperations::multiply: left matrix should be in NTT form");
        }
        if (!right.data[0].is_ntt_form()) {
            throw std::invalid_argument("MatrixOperations::multiply: right matrix should be in NTT form");
        }
    }

    template<typename T, typename U>
    void verify_correct_dimension(const matrix<T> &left, const matrix<U> &right) {
        if (left.cols != right.rows) {
            throw std::invalid_argument("MatrixOperations::multiply: left matrix cols != right matrix rows");
        }
    }

    // actual code:
    void MatrixOperations::transform(std::vector<seal::Plaintext> v, SplitPlaintextNTTFormMatrix &m) const {
        m.resize(v.size());
        std::transform(std::execution::par_unseq,
                       v.begin(),
                       v.end(),
                       m.begin(),
                       [&](seal::Plaintext &ptx) { return w_evaluator->split_plaintext(ptx); }
        );
    }

    void MatrixOperations::transform(std::vector<seal::Plaintext> v, CiphertextDefaultFormMatrix &m) const {
        m.resize(v.size());
        // TODO: into a parallel for loop.
        for (std::uint64_t i = 0; i < v.size(); i++) {
            w_evaluator->trivial_ciphertext(v[i], m[i]);
        }
    }

    void MatrixOperations::transform(const matrix<seal::Plaintext> &mat, matrix<seal::Ciphertext> &cmat) const {
        cmat.resize(mat.rows, mat.cols);
        for (uint64_t i = 0; i < mat.rows * mat.cols; i++) {
            w_evaluator->trivial_ciphertext(mat.data[i], cmat.data[i]);
        }
    }

    // only works when left_vec is 0/1..
    void MatrixOperations::left_multiply(const std::vector<std::uint64_t> &left_vec,
                                         const matrix<seal::Ciphertext> &mat,
                                         matrix<seal::Ciphertext> &result) const {
#ifdef DISTRIBICOM_DEBUG
        assert(left_vec.size() == mat.rows);
        for (auto item: left_vec) {
            assert(item <= 1);
        }
#endif
        result.resize(1, mat.cols);

        for (uint64_t k = 0; k < mat.cols; k++) {
            result(0, k) = seal::Ciphertext(w_evaluator->context);
            for (uint64_t j = 0; j < mat.rows; j++) {
                if (left_vec[j] == 0) {
                    continue;
                }
                w_evaluator->add(mat(j, k), result(0, k), result(0, k));
            }
        }
    }

    std::shared_ptr<MatrixOperations>
    MatrixOperations::Create(std::shared_ptr<EvaluatorWrapper> w_evaluator) {
        auto matops = std::make_shared<MatrixOperations>(w_evaluator);
        matops->start();
        return matops;
    }

    void MatrixOperations::right_multiply(std::vector<std::uint64_t> &dims, std::vector<SplitPlaintextNTTForm> &matrix,
                                          std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext>
                                          &result) {
#ifdef DISTRIBICOM_DEBUG
        // everything needs to be in NTT!
        for (auto &ptx: matrix) {
            assert(ptx[0].is_ntt_form());
        }

        assert(right_vec.size() == dims[COL]);
#endif

        seal::Ciphertext tmp;
        for (uint64_t j = 0; j < dims[ROW]; j++) {
            w_evaluator->mult(matrix[j], right_vec[0], result[j]);
            for (uint64_t k = 1; k < dims[COL]; k++) {

                w_evaluator->mult(
                        matrix[j + k * dims[ROW]],
                        right_vec[k],
                        tmp
                );

                w_evaluator->evaluator->add_inplace(result[j], tmp);

            }
        }
    }

    void MatrixOperations::right_multiply(std::vector<std::uint64_t> &dims, std::vector<seal::Ciphertext> &matrix,
                                          std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext>
                                          &result) {
#ifdef DISTRIBICOM_DEBUG
        // everything needs to be in NTT!
        for (auto &ctx: matrix) {
            assert(!ctx.is_ntt_form());
        }
        assert(right_vec.size() == dims[COL]);
#endif
        seal::Ciphertext tmp;
        for (uint64_t j = 0; j < dims[ROW]; j++) {
            w_evaluator->mult(matrix[j], right_vec[0], result[j]);
            for (uint64_t k = 1; k < dims[COL]; k++) {

                w_evaluator->mult(
                        matrix[j + k * dims[ROW]],
                        right_vec[k],
                        tmp
                );

                w_evaluator->evaluator->add_inplace(result[j], tmp);

            }
        }
    }

    void MatrixOperations::to_ntt(std::vector<seal::Plaintext> &m) const {
        std::for_each(std::execution::par_unseq, m.begin(), m.end(), [this](seal::Plaintext &ptx) {
            w_evaluator->evaluator->transform_to_ntt_inplace(ptx, this->w_evaluator->context.first_parms_id());
        });
    }


    void MatrixOperations::to_ntt(std::vector<seal::Ciphertext> &m) const {
        std::for_each(std::execution::par_unseq, m.begin(), m.end(), [this](seal::Ciphertext &ctx) {
            w_evaluator->evaluator->transform_to_ntt_inplace(ctx);
        });
    }

    void MatrixOperations::from_ntt(std::vector<seal::Ciphertext> &m) const {
        std::for_each(std::execution::par_unseq, m.begin(), m.end(), [this](seal::Ciphertext &ctx) {
            w_evaluator->evaluator->transform_from_ntt_inplace(ctx);
        });
    }


    void MatrixOperations::multiply(const matrix<seal::Ciphertext> &left,
                                    const matrix<seal::Ciphertext> &right,
                                    matrix<seal::Ciphertext> &result) const {

        if (left.data[0].is_ntt_form() || right.data[0].is_ntt_form()) {
            throw std::runtime_error("MatrixOperations::multiply: matrices should not be in NTT form");
        }

        seal::Ciphertext tmp;
        result.resize(left.rows, right.cols);
        mat_mult(left, right, result);
    }

    void MatrixOperations::multiply(const matrix<seal::Plaintext> &left,
                                    const matrix<seal::Ciphertext> &right,
                                    matrix<seal::Ciphertext> &result) const {
        verify_ptx_ctx_mat_mul_args(left, right);

        seal::Ciphertext tmp;
        result.resize(left.rows, right.cols);
        matrix<SplitPlaintextNTTForm> left_ntt(left.rows, left.cols);
        transform(left.data, left_ntt.data);

        multiply(left_ntt, right, result);
    }

    void MatrixOperations::multiply(const matrix<SplitPlaintextNTTForm> &left_ntt,
                                    const matrix<seal::Ciphertext> &right,
                                    matrix<seal::Ciphertext> &result) const {

        verify_spltptx_ctx_mat_mul_args(left_ntt, right);
        mat_mult(left_ntt, right, result);
    }

    template<typename U, typename V>
    void
    MatrixOperations::mat_mult(const matrix<U> &left, const matrix<V> &right,
                               matrix<seal::Ciphertext> &result) const {
        verify_correct_dimension(left, right);
        verify_not_empty_matrices(left, right);
        auto wg = std::make_shared<std::latch>(int(left.rows * right.cols));
        for (uint64_t i = 0; i < left.rows; ++i) {
            for (uint64_t j = 0; j < right.cols; ++j) {

                chan->write(
                        {
                                .f = [&, i, j]() { result(i, j) = mult_row(i, j, left, right); },
                                .wg = wg,
                        }
                );

            }
        }
        wg->wait();
    }

    template<typename U, typename V>
    std::shared_ptr<std::latch>
    MatrixOperations::async_mat_mult(const std::shared_ptr<matrix<U>> &left,
                                     const std::shared_ptr<matrix<V>> &right,
                                     std::shared_ptr<matrix<seal::Ciphertext>> &result) const {
        verify_correct_dimension(left, right);
        verify_not_empty_matrices(left, right);

        auto wg = std::make_shared<std::latch>(int(left->rows * right->cols));
        for (uint64_t i = 0; i < left->rows; ++i) {
            for (uint64_t j = 0; j < right->cols; ++j) {
                chan->write(
                        {
                                .f = [&, i, j]() { (*result)(i, j) = mult_row(i, j, left, right); },
                                .wg = wg,
                        }
                );
            }
        }
        return wg;
    }

    template<typename U, typename V>
    seal::Ciphertext MatrixOperations::mult_row(uint64_t i, uint64_t j, const matrix<U> &left,
                                                const matrix<V> &right) const {
        seal::Ciphertext tmp;
        seal::Ciphertext tmp_result(w_evaluator->context);
        // compiletime if, ensures the row multiplication is correct.
        if constexpr (

            // case 1: left  is ptx and right is ctx
                ((std::is_same_v<U, seal::Plaintext> || std::is_same_v<U, SplitPlaintextNTTForm>) &&
                 std::is_same_v<V, seal::Ciphertext>) ||

                // case 2: right is ptx and left is ctx
                ((std::is_same_v<V, seal::Plaintext> || std::is_same_v<V, SplitPlaintextNTTForm>) &&
                 std::is_same_v<U, seal::Ciphertext>)
                ) {
            w_evaluator->evaluator->transform_to_ntt_inplace(tmp_result);
        }

        for (uint64_t k = 0; k < left.cols; ++k) {
            w_evaluator->mult(left(i, k), right(k, j), tmp);
            w_evaluator->add(tmp, tmp_result, tmp_result);
        }
        return tmp_result;
    }


    void fill_rand_vec(uint64_t size, std::vector<std::uint64_t> &randvec) {
        randvec.resize(size);
        // using random seed:
        seal::Blake2xbPRNGFactory factory;
        auto gen = factory.create();
        for (auto &i: randvec) {
            i = gen->generate() % 2;
        }
    }

    void MatrixOperations::left_frievalds(const std::vector<std::uint64_t> &rand_vec,
                                          const matrix<seal::Ciphertext> &a,
                                          const matrix<seal::Ciphertext> &b,
                                          matrix<seal::Ciphertext> &result) const {
        matrix<seal::Ciphertext> tmp;
        left_multiply(rand_vec, a, tmp);
        multiply(tmp, b, result);
    }

    bool MatrixOperations::frievalds(const matrix<seal::Ciphertext> &a,
                                     const matrix<seal::Ciphertext> &b,
                                     const matrix<seal::Ciphertext> &c) const {
        // nxm * mxp = nxp
        if (a.rows != c.rows || b.cols != c.cols) {
            return false;
        }
        std::vector<std::uint64_t> rand_vec;
        fill_rand_vec(a.rows, rand_vec);

        matrix<seal::Ciphertext> left_vec;
        left_frievalds(rand_vec, a, b, left_vec);

        matrix<seal::Ciphertext> right_vec;
        left_multiply(rand_vec, c, right_vec);

        // subtracting the two vectors:
        for (std::uint64_t i = 0; i < left_vec.data.size(); i++) {
            w_evaluator->evaluator->sub_inplace(left_vec.data[i], right_vec.data[i]);
            if (!left_vec.data[i].is_transparent()) {
                return false;
            }
        }

        return true;
    }

    MatrixOperations::
    MatrixOperations(std::shared_ptr<EvaluatorWrapper> w_evaluator) : chan(), threads(), w_evaluator(w_evaluator) {
        chan = std::make_shared<concurrency::Channel<Task>>();
    }

    void MatrixOperations::start() {
        auto processor_count = std::thread::hardware_concurrency();

        // creating our multiplication worker.
        for (std::uint64_t i = 0; i < processor_count; ++i) {
            threads.emplace_back([&] {
                seal::Ciphertext tmp(w_evaluator->context);

                while (true) {
                    concurrency::Result<Task> r = chan->read();
                    if (!r.ok) {
                        return;
                    }
                    r.answer.f();
                    r.answer.wg->count_down();
                }
            });
        }
    }

    // TODO: reuse code with the following function.
    template<typename U>
    std::shared_ptr<std::latch>
    MatrixOperations::async_scalar_dot_product(const std::shared_ptr<std::vector<std::uint64_t>> &vec,
                                               const std::shared_ptr<matrix<U>> &mat,
                                               std::shared_ptr<matrix<seal::Ciphertext>> &result_vec) const {

        result_vec->resize(1, mat->cols);
        // todo: verify the dimensions are correct.

        auto wg = std::make_shared<std::latch>(int(mat->cols));
        for (uint64_t k = 0; k < mat->cols; k++) {
            chan->write(
                    {
                            .f = [&, k]() {
                                seal::Ciphertext tmp;
                                seal::Ciphertext rslt(w_evaluator->context);
                                for (uint64_t j = 0; j < mat->rows; j++) {
                                    w_evaluator->scalar_multiply((*vec)[j], (*mat)(j, k), tmp);
                                    w_evaluator->add(tmp, rslt, rslt);
                                }
                                (*result_vec)(0, k) = rslt;
                            },
                            .wg = wg,
                    }
            );

        }

        return wg;
    }

    template<typename U>
    std::shared_ptr<std::latch>
    MatrixOperations::async_scalar_dot_product(
            const std::shared_ptr<matrix<U>> &mat,
            const std::shared_ptr<std::vector<std::uint64_t>> &vec,
            std::shared_ptr<matrix<seal::Ciphertext>> &result_vec) const {

        result_vec->resize(mat->rows, 1);
        // todo: verify the dimensions are correct.

        auto wg = std::make_shared<std::latch>(int(mat->rows));
        for (uint64_t k = 0; k < mat->rows; k++) {
            chan->write(
                    {
                            .f= [&, k]() {
                                seal::Ciphertext tmp;
                                seal::Ciphertext rslt(w_evaluator->context);
                                for (uint64_t j = 0; j < mat->cols; j++) {
                                    w_evaluator->scalar_multiply((*vec)[j], (*mat)(k, j), tmp);
                                    w_evaluator->add(tmp, rslt, rslt);
                                }
                                (*result_vec)(k, 0) = rslt;
                            },
                            .wg = wg,
                    }
            );

        }

        return wg;
    }

}