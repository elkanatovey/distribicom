#pragma once
#include "pir.hpp"
#include "pir_server.hpp"

namespace multiplication_utils{
    class matrix_multiplier {
    public:
        std::shared_ptr<seal::Evaluator> evaluator_;
        seal::EncryptionParameters enc_params_; // SEAL parameters

        /***
         * multiply matrix by integer array
         * @param dims [num_of_cols, num_of_rows] of matrix
         * @param left_vec integer vector
         * @param matrix plaintext matrix to mult
         * @param result where to place result, not in ntt representation
         */
        void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec, Database& matrix,
                           std::vector<seal::Plaintext> &result);

        virtual void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                                   Database& matrix, std::vector<seal::Ciphertext> &result)=0;

        virtual void right_multiply(std::vector<std::uint64_t> &dims, Database& matrix,
                                    std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext> &result)=0;

        virtual void right_multiply(std::vector<std::uint64_t> &dims, std::vector<seal::Ciphertext> &matrix,
                                    std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext> &result)=0;
    };

    void foo();
}
