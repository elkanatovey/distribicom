#pragma once
#include "pir.hpp"
#include "pir_server.hpp"

namespace multiplication_utils{
    class matrix_multiplier {
    protected:
        std::shared_ptr<seal::Evaluator> evaluator_;
        seal::EncryptionParameters enc_params_; // SEAL parameters
        seal::SEALContext context_ = seal::SEALContext(seal::EncryptionParameters());
    public:

        /***
         * Creates and returns a an initialized matrix_multiplier
         * @param evaluator
         * @param enc_params
         * @return a matrix multiplier
         */
        static std::shared_ptr<matrix_multiplier> Create(std::shared_ptr<seal::Evaluator> evaluator,
                seal::EncryptionParameters enc_params);

        /***
         * multiply matrix by integer array using operations on plaintexts
         * @param dims [num_of_cols, num_of_rows] of matrix
         * @param left_vec integer vector
         * @param matrix plaintext matrix to mult
         * @param result where to place result, not in ntt representation
         */
        void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec, Database& matrix,
                           std::vector<seal::Plaintext> &result);

        /***
         * multiply matrix by integer array using operations on ciphertexts
         * @param dims [num_of_cols, num_of_rows] of matrix
         * @param left_vec integer vector
         * @param matrix plaintext matrix to mult
         * @param result where to place result, not in ntt representation
         */
        virtual void left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                                   Database& matrix, std::vector<seal::Ciphertext> &result);

//        virtual void right_multiply(std::vector<std::uint64_t> &dims, Database& matrix,
//                                    std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext> &result)=0;
//
//        virtual void right_multiply(std::vector<std::uint64_t> &dims, std::vector<seal::Ciphertext> &matrix,
//                                    std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext> &result)=0;

        matrix_multiplier(std::shared_ptr<seal::Evaluator> evaluator,
                          seal::EncryptionParameters enc_params):evaluator_(evaluator), enc_params_(enc_params)
                          {
            context_ = seal::SEALContext(enc_params, true);
                          };

        seal::Ciphertext get_trivial_encryption_of_zero() const;
    };

    void foo();

    /***
     * transform plaintexts to trivial (nonsafely encrypted) ciphertexts
     * @param plaintexts the plaintexts to copy
     * @param result the ciphertext vector to put the plaintexts in
     * @param evaluator
     */
    void plaintexts_to_trivial_ciphertexts(vector<seal::Plaintext> &plaintexts, vector<seal::Ciphertext> &result,
                                           seal::Ciphertext &trivial_zero, seal::Evaluator *evaluator);
}
