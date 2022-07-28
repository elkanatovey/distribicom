#include "matrix_multiplier_base.hpp"
#define COL 0
#define ROW 1

namespace multiplication_utils{
    void foo(){
        std::cout<<1<<std::endl;
    }

    void plaintexts_to_trivial_ciphertexts(vector<seal::Plaintext> &plaintexts, vector<seal::Ciphertext> &result,
                                           seal::Ciphertext &trivial_zero, seal::Evaluator *evaluator) {
        for(int i=0; i< plaintexts.size();i++){
            evaluator->add_plain(trivial_zero, plaintexts[i], result[i]);
        }
    };

    /**
 * take vector along with constant and do result+= vector*constant
 * @param right vector from above
 * @param left const
 * @param result place to add to
 */
    void multiply_add(std::uint64_t left, seal::Plaintext &right,
                      seal::Plaintext &result, uint64_t mod) {
        if(left==0){
            return;
        }
        auto coeff_count = right.coeff_count();
        for(int current_coeff =0; current_coeff < coeff_count; current_coeff++){
            result[current_coeff] +=(left * right[current_coeff]);
            if(result[current_coeff]>=mod){
                result[current_coeff]-=mod;
            }
        }

    }

    /*
     * note this only works when left=0/1
     */
    void multiply_add(std::uint64_t left, seal::Ciphertext &right,
                      seal::Ciphertext &result, seal::Evaluator* evaluator){
        if(left==0){
            return;
        }
        evaluator->add_inplace(result, right);
    }


    void matrix_multiplier::left_multiply(std::vector<std::uint64_t> &dims,
                                          vector<std::uint64_t> &left_vec, Database &matrix,
                                          vector<seal::Plaintext> &result) {
        for(uint64_t k = 0; k < dims[COL]; k++) {
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                multiply_add(left_vec[j], matrix[j + k * dims[ROW]], result[k], enc_params_.plain_modulus().value());
            }
        }

    }


    void
    matrix_multiplier::left_multiply(vector<std::uint64_t> &dims, vector<std::uint64_t> &left_vec, Database &matrix,
                                     vector<seal::Ciphertext> &result) {
        seal::Ciphertext trivial_zero = get_trivial_encryption_of_zero();

        std::vector<seal::Ciphertext> trivial_encrypted_matrix(matrix.size(), seal::Ciphertext(context_));
        plaintexts_to_trivial_ciphertexts(matrix, trivial_encrypted_matrix, trivial_zero, evaluator_.get());


        for(uint64_t k = 0; k < dims[COL]; k++) {
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                multiply_add(left_vec[j], trivial_encrypted_matrix[j + k * dims[ROW]], result[k], evaluator_.get());
            }
        }
    }

    seal::Ciphertext matrix_multiplier::get_trivial_encryption_of_zero() const {
        auto trivial_zero = seal::Ciphertext(context_);
        trivial_zero.resize(2);
        return trivial_zero;
    }

    std::shared_ptr<matrix_multiplier>
    matrix_multiplier::Create(std::shared_ptr<seal::Evaluator> evaluator, seal::EncryptionParameters enc_params) {
        auto multiplier = matrix_multiplier(evaluator, enc_params);
        return std::make_shared<matrix_multiplier>(std::move(multiplier));
    }

}