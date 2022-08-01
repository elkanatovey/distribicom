#include <seal/seal.h>
#include "test_utils.cpp"
#include "evaluator_wrapper.h"

int main() {
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
// associative test.

// commutative test.
}