#include "../test_utils.hpp"

int benchmarks(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto ptx = all->random_plaintext();
    auto ctx1 = all->random_ciphertext();
    auto ctx2 = all->random_ciphertext();
    auto ctx3 = all->random_ciphertext();

    auto repetitions = 10000;

    auto &ev = all->w_evaluator->evaluator;

    auto add_cipher = TestUtils::time_func([&] {
        for (int i = 0; i < repetitions; i++)
            ev->add(ctx1, ctx2, ctx3);
    }) / repetitions;
    std::cout << "addition-cipher-time:  " << add_cipher << std::endl;

    auto add_plain = TestUtils::time_func([&] {
        auto &ev = all->w_evaluator->evaluator;
        for (int i = 0; i < repetitions; i++)
            ev->add_plain(ctx1, ptx, ctx3);
    }) / repetitions;
    std::cout << "addition-plain-time:   " << add_plain << std::endl;

    auto mult_cipher = TestUtils::time_func([&] {
        for (int i = 0; i < repetitions; i++)
            ev->multiply(ctx1, ctx2, ctx3);
    }) / repetitions;
    std::cout << "mult-cipher-time:      " << mult_cipher << std::endl;

    ev->transform_to_ntt_inplace(ctx1);
    ev->transform_to_ntt_inplace(ptx, ctx1.parms_id());
    auto mult_plain = TestUtils::time_func([&] {
        for (int i = 0; i < repetitions; i++)
            ev->multiply_plain(ctx1, ptx, ctx3);
    }) / repetitions;
    std::cout << "mult-plain-time:       " << mult_plain << std::endl;

    ptx = all->random_plaintext();
    auto wev = all->w_evaluator;
    auto splt = wev->split_plaintext(ptx);
    auto preserving_mult_plain = TestUtils::time_func([&] {
        for (int i = 0; i < repetitions; i++)
            wev->mult(splt, ctx1, ctx3);
    }) / repetitions;
    std::cout << "preserving_mult_plain: " << preserving_mult_plain << std::endl;
    return 0;
}