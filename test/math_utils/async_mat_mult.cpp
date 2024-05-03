// This test is currently deprecated as async mult does not support Plaintext multiplication
#include "../test_utils.hpp"
#include "math_utils/matrix_operations.hpp"

void async_mult_test(const std::shared_ptr<TestUtils::CryptoObjects> &all);

void async_scalar_vec_mult(const std::shared_ptr<TestUtils::CryptoObjects> &all);

int async_mat_mult(int, char *[]) {
    // setup
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
    // test:
    async_mult_test(all);
    async_scalar_vec_mult(all);
    return 0;
}

void async_scalar_vec_mult(const std::shared_ptr<TestUtils::CryptoObjects> &all) {
    uint64_t rows = 2, cols = 2;
    auto eye = std::make_shared<math_utils::matrix<seal::Ciphertext>>(rows, cols);

    std::uint64_t val_in_eye = 2;
    std::uint64_t val_in_vec = 4;
    seal::Plaintext ptx;
    ptx = val_in_eye;

    all->encryptor.encrypt_symmetric(ptx, (*eye)(0, 0));
    all->encryptor.encrypt_symmetric(ptx, (*eye)(1, 1));

    seal::Plaintext zero;
    zero = 0;
    all->encryptor.encrypt_symmetric(zero, (*eye)(0, 1));
    all->encryptor.encrypt_symmetric(zero, (*eye)(1, 0));


    auto v = std::make_shared<std::vector<std::uint64_t>>();
    v->push_back(val_in_vec);
    v->push_back(val_in_vec);

    auto matops = math_utils::MatrixOperations::Create(all->w_evaluator);

    auto promise = matops->async_scalar_dot_product(eye, v);
    auto result = promise->get();


    assert(result->rows == 2);
    assert(result->cols == 1);
    for (const auto &ctx: result->data) {
        seal::Plaintext tmp;
        all->decryptor.decrypt(ctx, tmp);
        std::string tp = std::to_string(int(val_in_vec * val_in_eye));
        std::string txt_val = tmp.to_string();
        assert(txt_val == tp);
    }


    promise = matops->async_scalar_dot_product(v, eye);
    result = promise->get();

    assert(result->rows == 1);
    assert(result->cols == 2);
    for (const auto &ctx: result->data) {
        seal::Plaintext tmp;
        all->decryptor.decrypt(ctx, tmp);
        assert(tmp.to_string() == std::to_string(int(val_in_vec * val_in_eye)));
    }
}

void async_mult_test(const std::shared_ptr<TestUtils::CryptoObjects> &all) {
    uint64_t n = 25;
    uint64_t rows = n, cols = n;

    auto B = std::make_shared<math_utils::matrix<seal::Ciphertext>>(rows, cols);
    auto C = std::make_shared<math_utils::matrix<seal::Ciphertext>>();
    auto A_as_ptx = std::make_shared<math_utils::matrix<seal::Plaintext>>(rows, cols);

    for (uint64_t i = 0; i < rows * cols; ++i) {
        A_as_ptx->data[i] = all->random_plaintext();
        B->data[i] = all->random_ciphertext();
    }

    auto matops = math_utils::MatrixOperations::Create(all->w_evaluator);

    matops->to_ntt(B->data);
    TestUtils::time_func_print("mat-mult", [&matops, &A_as_ptx, &B, &C]() { matops->multiply(*A_as_ptx, *B, *C); });

    matops->to_ntt(A_as_ptx->data);
    auto p = matops->async_mat_mult<>(A_as_ptx, B);
    auto async_c = p->get();
    for (uint64_t i = 0; i < C->data.size(); ++i) {
        all->w_evaluator->evaluator->sub_inplace(C->data[i], async_c->data[i]);
        assert(C->data[i].is_transparent());
    }
}
