//
// Created by Jonathan Weiss on 28/07/2022.
//

#include "matrix_multiplier_base.hpp"
#include <seal/seal.h>
#include "test_utils.h"

void set_enc_params(uint32_t N, uint32_t logt, seal::EncryptionParameters &enc_params) {
    std::__1::cout << "Main: Generating SEAL parameters" << std::__1::endl;
    enc_params.set_poly_modulus_degree(N);
    enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
    enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));
}