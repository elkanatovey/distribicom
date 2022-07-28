#include <seal/seal.h>
#include "matrix_multiplier_base.hpp"
#include "test_utils.cpp"

int main(int argc, char *argv[]) {
    // sanity check
    multiplication_utils::foo();

    auto all = TestUtils::setup(TestUtils::SetupConfigs{
            .encryption_params_configs =  {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096,
                    .log_coefficient_modulus = 20
            },
            .rng_configs = {
                    .rng_type = TestUtils::blake,
            }
    });

    cout << "Main: generating matrices" << endl;

    std::uint64_t rows = 20;
    std::uint64_t cols = 4;
    std::vector<seal::Plaintext> matrix(rows * cols);

    std::vector<std::uint64_t> random_values(all.encryption_params.poly_modulus_degree(), 0ULL);
    for (auto &elem: matrix) {
        generate(random_values.begin(), random_values.end(), all.rng);
        all.encoder.encode(random_values, elem);
    }


    auto matrix_multiplier = multiplication_utils::matrix_multiplier::Create(all.evaluator, all.encryption_params);

    std::vector<std::uint64_t> left_vec(rows, 1);
    std::vector<seal::Plaintext> results(cols, seal::Plaintext(all.encryption_params.poly_modulus_degree()));

    std::vector<std::uint64_t> dims = {cols, rows};
    matrix_multiplier->left_multiply(dims, left_vec, matrix, results);
}
