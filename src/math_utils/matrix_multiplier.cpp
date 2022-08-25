#include "matrix_multiplier.hpp"
#include "defines.h"

namespace multiplication_utils {
    void foo() {
        std::cout << 1 << std::endl;
    }

    void matrix_multiplier::transform(std::vector<seal::Plaintext> v, SplitPlaintextNTTFormMatrix &m) const {
//        #ifdef MY_DEBUG
        assert(m.size() == v.size());
//        #endif

        for (uint64_t i = 0; i < v.size(); i++) {
            m[i] = w_evaluator->split_plaintext(v[i]);
        }
    }

    void matrix_multiplier::transform(std::vector<seal::Plaintext> v, CiphertextDefaultFormMatrix &m) const {
//        #ifdef DISTRIBICOM_DEBUG
        assert(m.size() == v.size());
//        #endif

        for (std::uint64_t i = 0; i < v.size(); i++) {
            w_evaluator->trivial_ciphertext(v[i], m[i]);
        }
    }


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
                                          std::vector<std::uint64_t> &left_vec, PlaintextDefaultFormMatrix &matrix,
                                          std::vector<seal::Plaintext> &result) {
        for (uint64_t k = 0; k < dims[COL]; k++) {
            result[k] = seal::Plaintext(w_evaluator->enc_params.poly_modulus_degree());
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                w_evaluator->multiply_add(left_vec[j], matrix[j + k * dims[ROW]], result[k]);
            }
        }
    }


    // this is the slow version of multiplication, needed to set the DB as a ciphertext DB.
    void
    matrix_multiplier::left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                                     PlaintextDefaultFormMatrix &matrix,
                                     std::vector<seal::Ciphertext> &result) {
        std::vector<seal::Ciphertext> trivial_encrypted_matrix(matrix.size(), seal::Ciphertext(w_evaluator->context));
        transform(matrix, trivial_encrypted_matrix);


        left_multiply(dims, left_vec, trivial_encrypted_matrix, result);
    }

    void matrix_multiplier::left_multiply(std::vector<std::uint64_t> &dims, std::vector<std::uint64_t> &left_vec,
                                          std::vector<seal::Ciphertext> &matrix, std::vector<seal::Ciphertext>
                                          &result) {

        for (uint64_t k = 0; k < dims[COL]; k++) {
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                w_evaluator->multiply_add(
                        left_vec[j],
                        matrix[j + k * dims[ROW]],
                        result[k]
                );
            }
        }
    }

    std::shared_ptr<matrix_multiplier>
    matrix_multiplier::Create(std::shared_ptr<EvaluatorWrapper> w_evaluator) {
        auto multiplier = matrix_multiplier(w_evaluator);
        return std::make_shared<matrix_multiplier>(std::move(multiplier));
    }

    void matrix_multiplier::right_multiply(std::vector<std::uint64_t> &dims, std::vector<SplitPlaintextNTTForm> &matrix,
                                           std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext>
                                           &result) {
//        #ifdef DISTRIBICOM_DEBUG
        // everything needs to be in NTT!
        for (auto &ptx: matrix) {
            assert(ptx[0].is_ntt_form());
        }

        assert(right_vec.size() == dims[COL]);
//        #endif

        seal::Ciphertext tmp;
        for (uint64_t j = 0; j < dims[ROW]; j++) {
            w_evaluator->mult_modified(matrix[j], right_vec[0], result[j]);
            for (uint64_t k = 1; k < dims[COL]; k++) {

                w_evaluator->mult_modified(
                        matrix[j + k * dims[ROW]],
                        right_vec[k],
                        tmp
                );

                w_evaluator->evaluator->add_inplace(result[j], tmp);

            }
        }
    }

    void matrix_multiplier::right_multiply(std::vector<std::uint64_t> &dims, std::vector<seal::Ciphertext> &matrix,
                                           std::vector<seal::Ciphertext> &right_vec, std::vector<seal::Ciphertext>
                                           &result) {
//#ifdef DISTRIBICOM_DEBUG
        // everything needs to be in NTT!
        for (auto &ctx: matrix) {
            assert(!ctx.is_ntt_form());
        }
//        assert(right_vec.size() == dims[COL]);

//#endif
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

    void matrix_multiplier::to_ntt(std::vector<seal::Ciphertext> &m) {
        for (auto &i: m) {
            w_evaluator->evaluator->transform_to_ntt_inplace(i);
        }
    }

    void matrix_multiplier::from_ntt(std::vector<seal::Ciphertext> &m) {
        for (auto &i: m) {
            w_evaluator->evaluator->transform_from_ntt_inplace(i);
        }
    }

}