#include "../test_utils.hpp"

// comments on the tests in their respective implementation.
void mult_slow_vs_modified_test();

void order_of_ops_test1();

void order_of_ops_test2();

int evaluator_wrapper_test(int, char *[]) {
    mult_slow_vs_modified_test();

    // commutative test.
    order_of_ops_test1();

    // associative test.
    order_of_ops_test2();

    return 0;
}

void mult_slow_vs_modified_test() {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto ptx = all->random_plaintext();
    auto ctx = all->random_ciphertext();

    seal::Ciphertext out1;
    auto ptx2 = ptx;
    auto ctx2 = ctx;
    all->w_evaluator->evaluator->transform_to_ntt_inplace(ctx2);
    all->w_evaluator->evaluator->transform_to_ntt_inplace(ptx2, all->seal_context.first_parms_id());
    all->w_evaluator->mult_reg(ptx2, ctx2, out1);

    seal::Ciphertext out2;
    all->w_evaluator->mult_slow(ptx, ctx, out2);
    all->w_evaluator->evaluator->transform_to_ntt_inplace(out2); // both results should be in ntt.

    all->w_evaluator->evaluator->sub_inplace(out1, out2);

    all->w_evaluator->evaluator->transform_from_ntt_inplace(out1);
    all->decryptor.decrypt(out1, ptx2);
    assert(ptx2.is_zero() && !out1.is_transparent());

    seal::Ciphertext out3;
    all->w_evaluator->evaluator->transform_to_ntt_inplace(ctx); // ntt expected.
    all->w_evaluator->mult_modified(ptx, ctx, out3);


    all->w_evaluator->evaluator->sub_inplace(out2, out3);

    assert(out2.is_transparent());
}


seal::Ciphertext create_ctx_multed_with_random_ptx(const std::shared_ptr<TestUtils::CryptoObjects> &all) {
    auto ctx1 = all->random_ciphertext();
    auto ptx1 = all->random_plaintext();
    all->w_evaluator->evaluator->transform_to_ntt_inplace(ctx1);
    all->w_evaluator->mult_modified(ptx1, ctx1, ctx1);
    all->w_evaluator->evaluator->transform_from_ntt_inplace(ctx1);
    return ctx1;
}

void order_of_ops_test1() {
    // testing ctx1p1 + ctx2p2 + ctx3 + ctx4 .... == ctx1p1+ ctx3 + ctx4 + ... + ctx2p2
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    seal::Ciphertext sum(all->w_evaluator->context);


    auto ctx1 = create_ctx_multed_with_random_ptx(all);
    all->w_evaluator->evaluator->add_inplace(sum, ctx1);

    auto ctx2 = create_ctx_multed_with_random_ptx(all);
    all->w_evaluator->evaluator->add_inplace(sum, ctx2);


    std::vector<seal::Ciphertext> ctxs(10);
    for (auto &p: ctxs) {
        p = all->random_ciphertext();
        all->w_evaluator->evaluator->add_inplace(sum, p);
    }


    for (auto i = ctxs.rbegin(); i != ctxs.rend(); ++i) {
        all->w_evaluator->evaluator->add_inplace(ctx1, *i);
    }
    all->w_evaluator->evaluator->add_inplace(ctx1, ctx2);

    // subtract the sum from the ctx1
    all->w_evaluator->evaluator->sub_inplace(ctx1, sum);

    // verify that the ctx1 is zero.
    seal::Plaintext ptx;
    all->decryptor.decrypt(ctx1, ptx);
    std::cout << ptx.to_string() << std::endl;
    assert(ctx1.is_transparent());
}

void order_of_ops_test2() {
    auto cnfgs = TestUtils::SetupConfigs{
            .encryption_params_configs = {
                    .scheme_type = seal::scheme_type::bfv,
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
    // in this test we check whether  g(ae + bf) + h(ce + df) == e(ga +hc) + f(gb + hd)
    // where g = 1 and h = 1, so:
    // ae+bf + ce+df = e(a+c) + f(b+d)

    seal::Plaintext
            a = all->random_plaintext(),
            b = all->random_plaintext(),
            c = all->random_plaintext(),
            d = all->random_plaintext();
    seal::Ciphertext
            e = all->random_ciphertext(),
            f = all->random_ciphertext();

    all->w_evaluator->evaluator->transform_to_ntt_inplace(e);
    all->w_evaluator->evaluator->transform_to_ntt_inplace(f);

    seal::Ciphertext ae, bf, ce, df;
    all->w_evaluator->mult_modified(a, e, ae);
    all->w_evaluator->mult_modified(b, f, bf);
    all->w_evaluator->mult_modified(c, e, ce);
    all->w_evaluator->mult_modified(d, f, df);

    seal::Ciphertext sum_left; // ae +bf + ce+df
    all->w_evaluator->evaluator->add(ce, df, sum_left);
    all->w_evaluator->evaluator->add(ae, sum_left, sum_left);
    all->w_evaluator->evaluator->add(bf, sum_left, sum_left);


    seal::Plaintext a_plus_c, b_plus_d;
    all->w_evaluator->add_plain(a, c, a_plus_c);
    all->w_evaluator->add_plain(b, d, b_plus_d);

    seal::Ciphertext e_x_a_plus_c, f_x_b_plus_d;
    all->w_evaluator->mult_modified(a_plus_c, e, e_x_a_plus_c);
    all->w_evaluator->mult_modified(b_plus_d, f, f_x_b_plus_d);
    seal::Ciphertext sum_right; // e(a+c) + f(b+d)
    all->w_evaluator->evaluator->add(e_x_a_plus_c, f_x_b_plus_d, sum_right);

    all->w_evaluator->evaluator->transform_from_ntt_inplace(sum_left);
    all->w_evaluator->evaluator->transform_from_ntt_inplace(sum_right);

    seal::Plaintext ptx_left, ptx_right;
    all->decryptor.decrypt(sum_left, ptx_left);
    all->decryptor.decrypt(sum_right, ptx_right);

    assert(ptx_left.to_string() == ptx_right.to_string());

    // subtract the sums
    assert(all->decryptor.invariant_noise_budget(sum_left) > 0);
    assert(all->decryptor.invariant_noise_budget(sum_right) > 0);


    all->w_evaluator->evaluator->sub_inplace(sum_left, sum_right);
    assert(all->decryptor.invariant_noise_budget(sum_left) > 0);

//  decrypting: should be zero and transparent.
    seal::Plaintext ptx;
    all->decryptor.decrypt(sum_left, ptx);
    assert(ptx.to_string() == "0");
    assert(sum_left.is_transparent());
}