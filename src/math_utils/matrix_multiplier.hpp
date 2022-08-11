#pragma once

#include "evaluator_wrapper.hpp"

namespace multiplication_utils {
    /***
     *  represents a matrix made out of SplitPlaintexts as its elements.
     */

    typedef std::vector<seal::Plaintext> PlaintextDefaultFormMatrix;
    typedef std::vector<SplitPlaintextNTTForm> SplitPlaintextNTTFormMatrix;
    typedef std::vector<seal::Ciphertext> CiphertextDefaultFormMatrix;

    class matrix_multiplier {
    private:
        explicit matrix_multiplier(std::shared_ptr<EvaluatorWrapper> w_evaluator) : w_evaluator(w_evaluator) {};
    protected:
        std::shared_ptr<EvaluatorWrapper> w_evaluator;
    public:

        /***
         * receives a plaintext matrix/ vector, and transforms it into a
         * splitPlainTextMatrix which is a vector of SplitPlaintextNTTForm in NTT form.
         */
        void transform(std::vector<seal::Plaintext> v, SplitPlaintextNTTFormMatrix &m) const;

        void transform(std::vector<seal::Plaintext> v, CiphertextDefaultFormMatrix &m) const;


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
        void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec, PlaintextDefaultFormMatrix &matrix,
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


    };

    void foo();
}
