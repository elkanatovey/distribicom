#include <seal/seal.h>
#include "matrix_multiplier_base.hpp"
#include "test_utils.cpp"

int main(int argc, char *argv[]) {
    // sanity check
    multiplication_utils::foo();

    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    cout << "Main: generating matrices" << endl;

    std::uint64_t rows = 20;
    std::uint64_t cols = 4;
    std::vector<seal::Plaintext> matrix(rows * cols);

    std::vector<std::uint64_t> random_values(all->encryption_params.poly_modulus_degree(), 0ULL);

    function<uint32_t(void)> random_val_generator;
    seal::Blake2xbPRNGFactory factory;
    auto gen = factory.create();
    auto f = [gen = std::move(gen)]() { return gen->generate() % 2; };
    for (auto &elem: matrix) {
        generate(random_values.begin(), random_values.end(), f);
        all->encoder.encode(random_values, elem);
    }


    auto matrix_multiplier = multiplication_utils::matrix_multiplier::Create(all->w_evaluator);

    std::vector<std::uint64_t> left_vec(rows, 1);
    std::vector<seal::Plaintext> results(cols, seal::Plaintext(all->encryption_params.poly_modulus_degree()));

    std::vector<std::uint64_t> dims = {cols, rows};
    matrix_multiplier->left_multiply(dims, left_vec, matrix, results);
}

