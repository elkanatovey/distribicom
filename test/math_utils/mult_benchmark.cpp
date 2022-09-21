#include "../test_utils.hpp"

// comments on the tests in their respective implementation.

int mult_benchmark(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);
    int iters = 10;
    auto ptx = all->random_plaintext();
    auto ctx = all->random_ciphertext();
    auto res = all->random_ciphertext();

    auto time = TestUtils::time_func([&]() {
        for (int i = 0; i < iters; ++i) {
            all->w_evaluator->mult_slow(ptx, ctx, res);
        }
    });
    std::cout << "mult_slow: " << time / iters << std::endl;
    auto mult_slow_time = time / iters;

    auto ctx2 = all->random_ciphertext();
    time = TestUtils::time_func([&]() {
        for (int i = 0; i < iters; ++i) {
            all->w_evaluator->mult(ctx, ctx2, res);
        }
    });
    std::cout << "mult:      " << time / iters << std::endl;
    auto mult_time = time / iters;

    time = TestUtils::time_func([&]() {
        for (int i = 0; i < iters; ++i) {
            all->w_evaluator->evaluator->multiply_plain(ctx, ptx, res);
        }
    });
    std::cout << "mult_plain: " << time / iters << std::endl;
    auto mult_plain_time = time / iters;

    std::cout << "comparing mult_slow to both mult and mult_plain: " << std::endl;
    std::cout << "mult_slow / mult:       " << double(mult_slow_time) / double(mult_time) << std::endl;
    std::cout << "mult_slow / mult_plain: " << double(mult_slow_time) / double(mult_plain_time) << std::endl;

    return 0;
}
