#include <seal/seal.h>
#include "test_utils.cpp"
#include "multiplier.h"

int main(int argc, char *argv[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto eval = multiplication_utils::EvaluatorWrapper::Create(all.evaluator, all.encryption_params);
    // TODO: write sanity check that multiplication version reach the same results.
}