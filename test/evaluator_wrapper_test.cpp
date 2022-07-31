#include <seal/seal.h>
#include "test_utils.cpp"
#include "evaluator_wrapper.h"

int main(int argc, char *argv[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto w_eval = multiplication_utils::EvaluatorWrapper::Create(all->shared_evaluator, all->encryption_params);
    auto ptx = all->random_plaintext();
    auto ctx = all->random_ciphertext();

    seal::Ciphertext out1;
    w_eval->mult_reg(ptx, ctx, out1);

    seal::Ciphertext out2;
    w_eval->mult_slow(ptx, ctx, out2);

    seal::Ciphertext out3;
    w_eval->mult_modified(ptx, ctx, out3);

    all->evaluator.sub_inplace(out2, out3);

    assert(out2.is_transparent());
// random ptx;
// random Enc(ptx2) = ctx;
// ptx*ctx == w_eval.mult_reg == w_eval mult_modified == w_eval.mult_slow;

// associative test.
// commutative test.
}