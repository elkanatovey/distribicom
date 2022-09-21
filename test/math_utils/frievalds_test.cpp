#include "../test_utils.hpp"
#include "matrix_multiplier.hpp"


int frievalds_test(int, char *[]) {
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

    std::uint64_t n = 20;
    std::uint64_t rows = n, cols = n;

    multiplication_utils::matrix<seal::Plaintext> DB(rows, cols);
    multiplication_utils::matrix<seal::Ciphertext> A, B(rows, cols), C;
    for (std::uint64_t i = 0; i < rows * cols; ++i) {
        DB.data[i] = all->random_plaintext();
        B.data[i] = all->random_ciphertext();
    }
    auto matops = multiplication_utils::matrix_multiplier::Create(all->w_evaluator);

    TestUtils::time_func_print("DB-transform", [&matops, &DB, &A]() { matops->transform(DB, A); });
    TestUtils::time_func_print("mat-mult", [&matops, &A, &B, &C]() { matops->multiply(A, B, C); });

    TestUtils::time_func_print("frievalds", [&matops, &A, &B, &C]() { assert(matops->frievalds(A, B, C)); });

    // Verifying Frievalds fails a bad C, such that C!= A * B
    for (int i = 0; i < 5; ++i) {
        C.data[0 + i * C.cols] = all->random_ciphertext();
    }

    TestUtils::time_func_print("frievalds", [&matops, &A, &B, &C]() { assert(!matops->frievalds(A, B, C)); });
    return 0;
}