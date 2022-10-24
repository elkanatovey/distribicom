#include "../test_utils.hpp"
#include "matrix_multiplier.hpp"

void frievalds_with_ptx_db();

int frievalds_test(int, char *[]) {
    frievalds_with_ptx_db();
    return 0;
}


void frievalds_with_ptx_db() {
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

    std::uint64_t n = 50;
    std::uint64_t rows = n, cols = n;

    multiplication_utils::matrix<seal::Ciphertext> A, B(rows, cols), C;
    multiplication_utils::matrix<seal::Plaintext> A_as_ptx(rows, cols);
    for (std::uint64_t i = 0; i < rows * cols; ++i) {
        A_as_ptx.data[i] = all->random_plaintext();
        B.data[i] = all->random_ciphertext();
    }
    auto matops = multiplication_utils::matrix_multiplier::Create(all->w_evaluator);
    TestUtils::time_func_print("A_as_ptx-transform to ctxs",
                               [&matops, &A_as_ptx, &A]() { matops->transform(A_as_ptx, A); });


    matops->to_ntt(B.data);
    TestUtils::time_func_print("mat-mult", [&matops, &A_as_ptx, &B, &C]() { matops->multiply(A_as_ptx, B, C); });


    matops->from_ntt(B.data);
    matops->from_ntt(C.data);
    TestUtils::time_func_print("frievalds", [&matops, &A, &B, &C]() { assert(matops->frievalds(A, B, C)); });
}

// 364146 /13897