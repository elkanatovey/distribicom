#include <seal/seal.h>
#include "matrix_multiplier.hpp"
#include "../test_utils.cpp"

int main() {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    cout << "Main: generating matrices" << endl;

    std::uint64_t rows = 30;
    std::uint64_t cols = 35;
    std::vector<uint64_t> dims = {cols, rows};

    // create some plaintext DB, and a random ciphertext vector.
    std::vector<seal::Plaintext> db(rows * cols);
    std::generate(db.begin(), db.end(), [&all]() { return all->random_plaintext(); });

    std::vector<seal::Ciphertext> right_vec(cols);
    std::generate(right_vec.begin(), right_vec.end(), [&all]() { return all->random_ciphertext(); });

    auto matops = multiplication_utils::matrix_multiplier::Create(all->w_evaluator);

    // transform it into CiphertextMatrix.
    multiplication_utils::CiphertextDefaultFormMatrix ciphertext_db(rows * cols,
                                                                    seal::Ciphertext(all->w_evaluator->context));
    matops->transform(db, ciphertext_db);

    // perform right mult transformed matrices.

    std::vector<seal::Ciphertext> ciphertext_db_results(rows);
    matops->right_multiply(dims, ciphertext_db, right_vec, ciphertext_db_results);


    // transform it into splitPlaintext.
    multiplication_utils::SplitPlaintextNTTFormMatrix split_ptx_db(rows * cols);
    matops->transform(db, split_ptx_db);
    for (auto &ctx: right_vec) {
        all->w_evaluator->evaluator->transform_to_ntt_inplace(ctx);
    }

    std::vector<seal::Ciphertext> splitdb_results(rows);
    matops->right_multiply(dims, split_ptx_db, right_vec, splitdb_results);
    // subtract results, expect zeros. (both modified mult, and slow mult should rsult in the same output)

    for (std::uint64_t i = 0; i < rows; i++) {
        all->w_evaluator->evaluator->transform_from_ntt_inplace(splitdb_results[i]);
        all->w_evaluator->evaluator->sub_inplace(splitdb_results[i], ciphertext_db_results[i]);
        assert(splitdb_results[i].is_transparent());
    }
}