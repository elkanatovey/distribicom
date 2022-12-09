//
// Created by Jonathan Weiss on 12/9/22.
//

#include "../test_utils.hpp"
#include "math_utils/matrix_operations.hpp"

int async_mat_mult(int, char *[]) {
    auto cnfgs = TestUtils::SetupConfigs{
            .encryption_params_configs = {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096 * 2,
                    .log_coefficient_modulus = 20,
            },
            .pir_params_configs = {
                    .number_of_items = 2048,
                    .size_per_item = 288,
                    .dimensions= 2,
                    .use_symmetric = false,
                    .use_batching = true,
                    .use_recursive_mod_switching = true,
            },
    };
    auto all = TestUtils::setup(cnfgs);

    std::uint64_t n = 10;
    std::uint64_t rows = n, cols = n;


    auto A = std::make_shared<math_utils::matrix<seal::Ciphertext>>();
    auto B = std::make_shared<math_utils::matrix<seal::Ciphertext>>(rows, cols);
    auto C = std::make_shared<math_utils::matrix<seal::Ciphertext>>();
    auto A_as_ptx = std::make_shared<math_utils::matrix<seal::Plaintext>>(rows, cols);

    for (std::uint64_t i = 0; i < rows * cols; ++i) {
        A_as_ptx->data[i] = all->random_plaintext();
        B->data[i] = all->random_ciphertext();
    }

    auto matops = math_utils::MatrixOperations::Create(all->w_evaluator);
    TestUtils::time_func_print("A_as_ptx-transform to ctxs",
                               [&matops, &A_as_ptx, &A]() { matops->transform((*A_as_ptx), (*A)); });


    matops->to_ntt(B->data);
    TestUtils::time_func_print("mat-mult", [&matops, &A_as_ptx, &B, &C]() { matops->multiply(*A_as_ptx, *B, *C); });


    auto p = matops->async_mat_mult<seal::Plaintext, seal::Ciphertext>(A_as_ptx, B);
    auto async_c = p->get();

    for (std::uint64_t i = 0; i < C->data.size(); ++i) {
        all->w_evaluator->evaluator->sub_inplace(C->data[i], async_c->data[i]);
        assert(C->data[i].is_transparent());
    }
}
