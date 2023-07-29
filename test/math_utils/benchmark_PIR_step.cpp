#include "../test_utils.hpp"

int benchmark_PIR_step(int, char *[]) {

    concurrency::num_cpus = 12;
    std::vector<std::pair<int, int>> db_size = {
            {15, 15}, // 2^13
            {29, 29}, // 2^15
            {42, 41},// 2^16
            {82, 82},// 2^18
            {164, 164},// 2^20
            {232, 232}, // 2^21
    };

    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);
    auto m = math_utils::MatrixOperations::Create(all->w_evaluator);

    for (auto &i: db_size) {
        // how to get the first element of a tuple?
        auto rows = i.first;
        auto cols = i.second;

        // create plaintext matrix (using the same value in all fields, shouldn't matter)
        auto db = std::make_shared<math_utils::matrix<seal::Plaintext>>(rows, cols, all->random_plaintext());

        // create random query vector (using the same value in all fields, shouldn't matter)
        math_utils::matrix<seal::Ciphertext> query_right(cols, 1, all->random_ciphertext());


        m->to_ntt(query_right.data);
        math_utils::matrix<seal::Ciphertext> res;

        auto time = TestUtils::time_func([&]() {
            for (int j = 0; j < 10; ++j) {
                m->multiply(*db, query_right, res);
            }
        });
        m->from_ntt(res.data);

        math_utils::matrix<seal::Ciphertext> tmp;
        math_utils::matrix<seal::Ciphertext> query_left(1, rows, all->random_ciphertext());

        auto time2 = TestUtils::time_func([&]() {
            for (int j = 0; j < 10; ++j) {
                m->multiply(query_left, res, tmp);
            }
        });
        std::cout << "rows: " << rows << ", cols: " << cols << ", PIR1 time: " << time / 10 << " " << "PIR2 time: "
                  << time2 / 10 << " ns/op" << std::endl;
    }


    return 0;
}