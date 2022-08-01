#include <seal/seal.h>
#include "matrix_multiplier_base.hpp"
#include "test_utils.cpp"

int main(int argc, char *argv[]) {
    // sanity check
    multiplication_utils::foo();

    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    cout << "Main: generating matrices" << endl;

    std::uint64_t rows = 2;
    std::uint64_t cols = 2;
    std::vector<seal::Plaintext> matrix(rows * cols);

    for (auto &a_ij: matrix) {
        a_ij = all->random_plaintext();
    }


    // test with [1,1]
    auto matrix_multiplier = multiplication_utils::matrix_multiplier::Create(all->w_evaluator);

    std::vector<std::uint64_t> left_vec(rows, 1);
    std::vector<seal::Plaintext> result(cols, seal::Plaintext(all->encryption_params.poly_modulus_degree()));

    std::vector<std::uint64_t> dims = {cols, rows};
    matrix_multiplier->left_multiply(dims, left_vec, matrix, result);

    seal::Plaintext r0;
    all->w_evaluator->add_plain(matrix[0], matrix[1], r0);
    assert(r0.to_string() == result[0].to_string());

    seal::Plaintext r1;
    all->w_evaluator->add_plain(matrix[2], matrix[3], r1);
    assert(r1.to_string() == result[1].to_string());


    // test with [0, 1]
//    std::vector<seal::Plaintext> result2(cols, seal::Plaintext(all->encryption_params.poly_modulus_degree()));
    left_vec[0] = 0; // [0, 1]
    matrix_multiplier->left_multiply(dims, left_vec, matrix, result);

    r0 = matrix[1];
    assert(r0.to_string() == result[0].to_string());

    r1 = matrix[3];
    assert(r1.to_string() == result[1].to_string());
}

