#include "matrix_multiplier_base.hpp"

#define COL 0
#define ROW 1

namespace multiplication_utils {
    void foo() {
        std::cout << 1 << std::endl;
    }

    void plaintexts_to_trivial_ciphertexts(vector<seal::Plaintext> &plaintexts, vector<seal::Ciphertext> &result,
                                           seal::Ciphertext &trivial_zero, seal::Evaluator *evaluator) {
        for (std::uint64_t i = 0; i < plaintexts.size(); i++) {
            evaluator->add_plain(trivial_zero, plaintexts[i], result[i]);
        }
    };


    /*
     * note this only works when left=0/1
     */
    // TODO: get rid of this.
    void multiply_add(std::uint64_t left, seal::Ciphertext &right,
                      seal::Ciphertext &result, seal::Evaluator *evaluator) {
        if (left == 0) {
            return;
        }
        assert(left == 1);
        evaluator->add_inplace(result, right);
    }


    void matrix_multiplier::left_multiply(std::vector<std::uint64_t> &dims,
                                          vector<std::uint64_t> &left_vec, Database &matrix,
                                          vector<seal::Plaintext> &result) {
        for (uint64_t k = 0; k < dims[COL]; k++) {
            result[k] = seal::Plaintext(w_evaluator->enc_params.poly_modulus_degree());
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                w_evaluator->multiply_add(left_vec[j], matrix[j + k * dims[ROW]], result[k]);
            }
        }

    }


    // this is the slow version of multiplication, needed to set the DB as a ciphertext DB.
    void
    matrix_multiplier::left_multiply(vector<std::uint64_t> &dims, vector<std::uint64_t> &left_vec, Database &matrix,
                                     vector<seal::Ciphertext> &result) {
        seal::Ciphertext trivial_zero = get_trivial_encryption_of_zero();

        std::vector<seal::Ciphertext> trivial_encrypted_matrix(matrix.size(), seal::Ciphertext(w_evaluator->context));
        plaintexts_to_trivial_ciphertexts(matrix, trivial_encrypted_matrix, trivial_zero, w_evaluator->evaluator.get());


        for (uint64_t k = 0; k < dims[COL]; k++) {
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                w_evaluator->multiply_add(
                        left_vec[j],
                        trivial_encrypted_matrix[j + k * dims[ROW]],
                        result[k]
                );
            }
        }
    }

    seal::Ciphertext matrix_multiplier::get_trivial_encryption_of_zero() const {
        auto trivial_zero = seal::Ciphertext(w_evaluator->context);
        trivial_zero.resize(2);
        return trivial_zero;
    }

    std::shared_ptr<matrix_multiplier>
    matrix_multiplier::Create(std::shared_ptr<EvaluatorWrapper> w_evaluator) {
        auto multiplier = matrix_multiplier(w_evaluator);
        return std::make_shared<matrix_multiplier>(std::move(multiplier));
    }

}