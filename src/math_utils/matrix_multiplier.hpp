#pragma once

#include "evaluator_wrapper.hpp"
#include "matrix.h"
#include "channel.h"


namespace multiplication_utils {
    /***
     *  represents a matrix made out of SplitPlaintexts as its elements.
     */

    typedef std::vector<seal::Plaintext> PlaintextDefaultFormMatrix;
    typedef std::vector<SplitPlaintextNTTForm> SplitPlaintextNTTFormMatrix;
    typedef std::vector<seal::Ciphertext> CiphertextDefaultFormMatrix;

    // represents a task to be executed by a thread
    struct task {
        int task_type; // todo: into enum. state the type of the task..
        // the WaitGroup that the issuer of the task is waiting on.
        std::shared_ptr<WaitGroup> wg;
        // compute details.
        std::uint64_t row;
        std::uint64_t col;
        std::uint64_t n; // number of elements in a row or column.

        // either we have a plain left
        const matrix<SplitPlaintextNTTForm> *left_ntt;
        // or we have a ctx left.
        const matrix<seal::Ciphertext> *left_ctx;

        // the result and the right matrix are always ctx matrices.
        const matrix<seal::Ciphertext> *right;
        matrix<seal::Ciphertext> *result;
    };

// TODO: this is not a class name style. and maybe change it to MatrixOperations
    class matrix_multiplier {
    protected:
        std::shared_ptr<EvaluatorWrapper> w_evaluator;
    private:
        std::shared_ptr<Channel<task>> chan;
        std::vector<std::thread> threads;


    public:
        explicit matrix_multiplier(std::shared_ptr<EvaluatorWrapper> w_evaluator);

        ~matrix_multiplier() {
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
         * Creates and returns a an initialized matrix_multiplier
         * @param evaluator
         * @param enc_params
         * @return a matrix multiplier
         */
        static std::shared_ptr<matrix_multiplier> Create(std::shared_ptr<EvaluatorWrapper> w_evaluator);

        /***
         * multiply matrix by integer array using operations on plaintexts
         * @param dims [num_of_cols, num_of_rows] of matrix
         * @param left_vec integer vector (row vector: [1,2,0,4,...])
         * @param matrix plaintext matrix to mult
         * @param result where to place result, not in ntt representation
         */
        void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                           PlaintextDefaultFormMatrix &matrix,
                           std::vector<seal::Plaintext> &result);


        /***
         * multiply matrix by integer array using operations on ciphertext.
         * Note, this should be called on the second side of the freivalds expression.
         * (r*A)*b - {r*(A*B)}, where A*B is ciphertext.
         */
        void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                           CiphertextDefaultFormMatrix &matrix, std::vector<seal::Ciphertext> &result);

        /***
         *
         * multiply matrix by integer array using operations on ciphertexts
         * Note, left where we have plaintext*ctx done via mult_slow.
         * @param dims [num_of_cols, num_of_rows] of matrix
         * @param left_vec integer vector
         * @param matrix plaintext matrix to mult
         * @param result where to place result, not in ntt representation
         */
        virtual void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                                   PlaintextDefaultFormMatrix &matrix, std::vector<seal::Ciphertext> &result);

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
                           const matrix <seal::Ciphertext> &mat,
                           matrix <seal::Ciphertext> &result) const;

        /**
         * Will transform the mat into trivial ciphertexts
         * @param left_vec
         * @param mat
         * @param result
         */
        void left_multiply(const std::vector<std::uint64_t> &left_vec,
                           const matrix <seal::Plaintext> &mat,
                           matrix <seal::Ciphertext> &result) const;

        void
        multiply(const matrix<seal::Ciphertext> &left, const matrix<seal::Ciphertext> &right,
                 matrix<seal::Ciphertext> &result) const;

        void
        multiply(const matrix<seal::Plaintext> &left, const matrix<seal::Ciphertext> &right,
                 matrix<seal::Ciphertext> &result) const;

        bool frievalds(const matrix<seal::Ciphertext> &A, const matrix<seal::Ciphertext> &B,
                       const matrix<seal::Ciphertext> &C) const;

    private:
        void
        left_frievalds(const std::vector<std::uint64_t> &rand_vec,
                       const matrix<seal::Ciphertext> &a,
                       const matrix<seal::Ciphertext> &b,
                       matrix<seal::Ciphertext> &result) const;
    };

    void foo();

}
