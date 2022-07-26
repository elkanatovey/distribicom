#include "matrix_multiplier_base.hpp"
#define COL 0
#define ROW 1

namespace multiplication_utils{
    void foo(){
        std::cout<<1<<std::endl;
    };

    void multiply_add(std::uint64_t left, seal::Plaintext &right,
                      seal::Plaintext &result, uint64_t mod);


    void matrix_multiplier::left_multiply(std::vector<std::uint64_t> &dims,
                                          vector<std::uint64_t> &left_vec, Database &matrix,
                                          vector<seal::Plaintext> &result) {
        for(uint64_t k = 0; k < dims[COL]; k++) {
            for (uint64_t j = 0; j < dims[ROW]; j++) {
                multiply_add(left_vec[j], matrix[j + k * dims[ROW]], result[k], enc_params_.plain_modulus().value());
            }
        }

    }

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
}