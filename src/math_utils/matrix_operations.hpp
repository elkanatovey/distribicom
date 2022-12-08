#pragma once

#include <latch>
#include "evaluator_wrapper.hpp"
#include "matrix.h"
#include "concurrency/channel.hpp"


namespace math_utils {
    /***
     *  represents a matrix made out of SplitPlaintexts as its elements.
     */

    typedef std::vector<seal::Plaintext> PlaintextDefaultFormMatrix;
    typedef std::vector<SplitPlaintextNTTForm> SplitPlaintextNTTFormMatrix;
    typedef std::vector<seal::Ciphertext> CiphertextDefaultFormMatrix;

    struct Task {
        std::function<void()> f;
        std::shared_ptr<std::latch> wg;
    };

    class MatrixOperations {
    protected:

    private:
        std::shared_ptr<concurrency::Channel<Task>> chan;
        std::vector<std::thread> threads;

#ifdef DISTRIBICOM_DEBUG
    public:
        std::shared_ptr<EvaluatorWrapper> w_evaluator;
#else
        std::shared_ptr<EvaluatorWrapper> w_evaluator;
        public:
#endif

        explicit MatrixOperations(std::shared_ptr<EvaluatorWrapper> w_evaluator);

        ~MatrixOperations() {
            chan->close();
            for (auto &t: threads) {
                t.join();
            }
        }

        void start();

        /***
         * receives a plaintext matrix/ vector, and transforms it into a
         * splitPlainTextMatrix which is a vector of SplitPlaintextNTTForm in NTT form.
         */
        void transform(std::vector<seal::Plaintext> v, SplitPlaintextNTTFormMatrix &m) const;

        void transform(std::vector<seal::Plaintext> v, CiphertextDefaultFormMatrix &m) const;

        void transform(const matrix<seal::Plaintext> &mat, matrix<seal::Ciphertext> &cmat) const;

        /***
         * Creates and returns a an initialized MatrixOperations
         * @param evaluator
         * @param enc_params
         * @return a matrix multiplier
         */
        static std::shared_ptr<MatrixOperations> Create(std::shared_ptr<EvaluatorWrapper> w_evaluator);

        /***
         *
         * @param dims
         * @param matrix
         * @param right_vec
         * @param result
         */
        void right_multiply(std::vector<std::uint64_t> &dims, SplitPlaintextNTTFormMatrix &matrix,
                            std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext> &result);

        /***
         * @param dims
         * @param matrix
         * @param right_vec
         * @param result
         */
        void right_multiply(std::vector<std::uint64_t> &dims, std::vector<seal::Ciphertext> &matrix,
                            std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext> &result);

        void to_ntt(std::vector<seal::Ciphertext> &m) const;


        void from_ntt(std::vector<seal::Ciphertext> &m) const;

        // working with matrices:
        void left_multiply(const std::vector<std::uint64_t> &left_vec,
                           const matrix<seal::Ciphertext> &mat,
                           matrix<seal::Ciphertext> &result) const;


        /***
         * performs matrix-multiplication.
         * will resize the result matrix to match the expected matrix output.
         * @param left a CTX matrix with its elements in NTT form.
         * @param right a CTX matrix with its elements in NTT form.
         * @param result a CTX matrix with its elements in NTT form.
         */
        void multiply(const matrix<seal::Ciphertext> &left, const matrix<seal::Ciphertext> &right,
                      matrix<seal::Ciphertext> &result) const;

        /**
         * performs threaded matrix-multiplication.
         * will resize the result matrix to match the expected matrix output.
         * @param left a PTX matrix with its elements NOT in NTT form.
         * @param right a CTX matrix with its elements in NTT form.
         * @param result a CTX matrix with its elements in NTT form.
         */
        void multiply(const matrix<seal::Plaintext> &left, const matrix<seal::Ciphertext> &right,
                      matrix<seal::Ciphertext> &result) const;

        /***
         * performs threaded matrix-multiplication.
         * will resize the result matrix to match the expected matrix output.
         * @param left_ntt a PTX matrix with its elements split and in NTT form.
         * @param right a CTX matrix with its elements in NTT form.
         * @param result a CTX matrix with its elements in NTT form.
         */
        void multiply(const matrix<SplitPlaintextNTTForm> &left_ntt,
                      const matrix<seal::Ciphertext> &right,
                      matrix<seal::Ciphertext> &result) const;

        bool frievalds(const matrix<seal::Ciphertext> &A, const matrix<seal::Ciphertext> &B,
                       const matrix<seal::Ciphertext> &C) const;

        template<typename U, typename V>
        seal::Ciphertext mult_row(uint64_t row, uint64_t col, const matrix<U> &left, const matrix<V> &right) const;

        template<typename U, typename V>
        void
        mat_mult(const matrix<U> &left, const matrix<V> &right,
                 matrix<seal::Ciphertext> &result) const;

        /***
         * usage: Note that this is an async function, it'll return fast without anything in result.
         *        Before you access result you must wait on the latch!
         * @tparam U
         * @tparam V
         * @param left
         * @param right
         * @param result
         * @return
         */
        template<typename U, typename V>
        std::shared_ptr<std::latch>
        async_mat_mult(const std::shared_ptr<matrix<U>> &left,
                       const std::shared_ptr<matrix<V>> &right,
                       std::shared_ptr<matrix<seal::Ciphertext>> &result) const;

        /***
         * Scalar dot product is used to perform Frievalds algorithm.
         * To ensure integrity of the computations any addition is performed between two ciphertexts.
         * furthermore, because plaintexts do not support multiplication we rely on Trivial multiplication.
         *
         * usage: Note that this is an async function, it'll return fast without anything in result_vec.
         *        Before you access result_vec you must wait on the latch!
         * @tparam U either a Ciphertext or a Plaintext
         * @param vec a vector of uints.
         * @param mat a matrix of type U (either Ciphertext or Plaintext).
         * @param result_vec a vector of Ciphertexts.
         */
        template<typename U>
        std::shared_ptr<std::latch>
        // TODO: instead of latch something smarter - waits and after the first wait is done it'll use atomic checks.
        async_scalar_dot_product(const std::shared_ptr<std::vector<std::uint64_t>> &vec,
                                 const std::shared_ptr<matrix<U>> &mat,
                                 std::shared_ptr<matrix<seal::Ciphertext>> &result_vec) const;

        /***
         *
         * @param mat
         * @param vec
         * @param result_vec
         */
        template<typename U>
        std::shared_ptr<std::latch> async_scalar_dot_product(
                const std::shared_ptr<matrix<U>> &mat,
                const std::shared_ptr<std::vector<std::uint64_t>> &vec,
                std::shared_ptr<matrix<seal::Ciphertext>> &result_vec) const;

    private:
        void
        left_frievalds(const std::vector<std::uint64_t> &rand_vec,
                       const matrix<seal::Ciphertext> &a,
                       const matrix<seal::Ciphertext> &b,
                       matrix<seal::Ciphertext> &result) const;

        void to_ntt(std::vector<seal::Plaintext> &m) const;
    };

}
