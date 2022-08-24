#include <query_expander.hpp>
#include "pir.hpp"
#include "pir_client.hpp"
#include "../test_utils.hpp"
#include "master.hpp"


// Throws on failure:
void correct_expansion_test(TestUtils::SetupConfigs);

void expanding_full_dimension_query(TestUtils::SetupConfigs);

int query_expander_test(int, char *[]) {
    auto cnfgs = TestUtils::SetupConfigs{
            .encryption_params_configs = {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096,
                    .log_coefficient_modulus = 20,
            },
            .pir_params_configs = {
                    .number_of_items = 2048,
                    .size_per_item = 288,
                    .dimensions= 2,
                    .use_symmetric = false,
                    .use_batching = true,
                    .use_recursive_mod_switching = true,
            },
    };
    expanding_full_dimension_query(cnfgs);
    cnfgs.pir_params_configs.dimensions = 1;
//    correct_expansion_test(cnfgs);
    return 0;
}

void correct_expansion_test(TestUtils::SetupConfigs cnfgs) {
    auto all = TestUtils::setup(cnfgs);

    // Initialize PIR client....
    PIRClient client(all->encryption_params, all->pir_params);
    seal::GaloisKeys galois_keys = client.generate_galois_keys();

    std::uint64_t n_i = all->pir_params.nvec[0];
    auto expander = multiplication_utils::QueryExpander::Create(all->encryption_params);

    for (std::uint64_t k = 0; k < all->pir_params.ele_num; ++k) {
        std::uint64_t ele_index = k;
        std::uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
        std::cout << "Main: element index = " << ele_index << " from [0, "
                  << all->pir_params.ele_num - 1 << "]" << std::endl;
        PirQuery query = client.generate_query(index);
        std::cout << "Main: query generated" << std::endl;

        std::vector<seal::Ciphertext> expanded_query = expander->__expand_query(query[0][0], n_i, galois_keys);
        std::cout << "Main: query expanded" << std::endl;

        assert(expanded_query.size() == n_i);

        std::cout << "Main: checking expansion" << std::endl;
        std::stringstream o;
        for (size_t i = 0; i < expanded_query.size(); i++) {
            seal::Plaintext decryption = client.decrypt(expanded_query.at(i));

            if (decryption.is_zero() && index != i) {
                continue;
            }
            else if (decryption.is_zero()) {
                o << "Found zero where index should be";
                throw std::runtime_error(o.str());
            }
            else if (std::stoi(decryption.to_string()) != 1) {
                o << "Query vector at index " << index
                  << " should be 1 but is instead " << decryption.to_string();
                throw std::runtime_error(o.str());
            }
            else {
                if (decryption.to_string() != "1") {
                    o << "Query vector at index " << index << " is not 1";
                    throw std::runtime_error(o.str());
                }
                std::cout << "Query vector at index " << index << " is "
                          << decryption.to_string() << std::endl;
            }
        }
    }
}

// helper func
std::shared_ptr<Database> gen_db(const shared_ptr<TestUtils::CryptoObjects> &all);

// todo: add to mat-ops calss.
void to_ntt(shared_ptr<TestUtils::CryptoObjects> all, multiplication_utils::CiphertextDefaultFormMatrix &m) {
    for (auto &i: m) {
        all->w_evaluator->evaluator->transform_to_ntt_inplace(i);
    }
}

void from_ntt(shared_ptr<TestUtils::CryptoObjects> all, std::vector<seal::Ciphertext> &m) {
    for (auto &i: m) {
        all->w_evaluator->evaluator->transform_from_ntt_inplace(i);
    }
}

// this test creates two (encrypted) e_0 vectors from the basic base. and multiply them with the matrix and except to
// find the element mat[0][0] in the result.
void expanding_full_dimension_query(TestUtils::SetupConfigs cnfgs) {
    assert(cnfgs.pir_params_configs.dimensions == 2);
    auto all = TestUtils::setup(cnfgs);


    auto db_ptr = gen_db(all);


    PIRClient client(all->encryption_params, all->pir_params);
    seal::GaloisKeys galois_keys = client.generate_galois_keys();
    std::uint64_t dim0_size = all->pir_params.nvec[0];
    std::uint64_t dim1_size = all->pir_params.nvec[1];

    std::uint64_t ele_index = 0;
    std::uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    std::cout << "Main: element index = " << ele_index << " from [0, "
              << all->pir_params.ele_num - 1 << "]" << std::endl;
    PirQuery query = client.generate_query(index);

    auto expander = multiplication_utils::QueryExpander::Create(all->encryption_params);

    auto expanded_query_dim_0 = expander->expand_query(query[0], dim0_size, galois_keys);
    auto expanded_query_dim_1 = expander->expand_query(query[1], dim1_size, galois_keys);
    // mult it.

    auto matops = multiplication_utils::matrix_multiplier::Create(all->w_evaluator);
    multiplication_utils::SplitPlaintextNTTFormMatrix splittx_db(db_ptr->size());
    matops->transform(*db_ptr, splittx_db);

    std::vector<seal::Ciphertext> matXv0(expanded_query_dim_0.size());
    std::vector<std::uint64_t> dims = {dim0_size, dim0_size};
    to_ntt(all, expanded_query_dim_0);
    matops->right_multiply(dims, splittx_db, expanded_query_dim_0, matXv0);


    from_ntt(all, matXv0);
    for (int i = 0; i < matXv0.size(); ++i) {
        assert(client.decrypt(matXv0[i]).to_string() == db_ptr->at(i).to_string()); // assert row was taken.
    }

    for (auto &i: expanded_query_dim_1) {
        std::cout << client.decrypt(i).to_string() << std::endl;
    }

    // because the expanded vector has a single element that is not 0, i'd expect the result to contain one element from the db
    std::vector<seal::Ciphertext> reduced_result(1);
    dims = {dim0_size, 1};
    matops->right_multiply(dims, matXv0, expanded_query_dim_1, reduced_result);
    auto out_string = client.decrypt(reduced_result.at(0)).to_string();
    for (int i = 0; i < db_ptr->size(); ++i) {
        if (db_ptr->at(i).to_string() == out_string) {
            return; // std::cout << "found match" << std::endl;
        }
    }
    throw std::runtime_error("could not find match");
}

std::shared_ptr<Database> gen_db(const shared_ptr<TestUtils::CryptoObjects> &all) {
    auto size_per_item = all->pir_params.ele_size;
    auto number_of_items = all->pir_params.ele_num;

    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
// fill db:
    seal::Blake2xbPRNGFactory factory;
    auto gen = factory.create();
    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            auto val = gen->generate() % 256;
            db.get()[(i * size_per_item) + j] = val;
        }
    }

    Master server(all->encryption_params, all->pir_params);

    server.set_database(move(db), number_of_items, size_per_item);
    return server.get_db();
}
