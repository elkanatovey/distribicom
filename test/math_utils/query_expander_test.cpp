#include <query_expander.hpp>
#include "pir.hpp"
#include "pir_client.hpp"
#include "../test_utils.hpp"

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
    correct_expansion_test(cnfgs);
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

void expanding_full_dimension_query(TestUtils::SetupConfigs cnfgs) {
    cnfgs.pir_params_configs.dimensions = 2;
    // need to expand a single query, and multiply it with some DB.
    // basically mult once from left and once from the right.
    // and expect a specific element.
    auto all = TestUtils::setup(cnfgs);

    PIRClient client(all->encryption_params, all->pir_params);
    seal::GaloisKeys galois_keys = client.generate_galois_keys();
    std::uint64_t n_0 = all->pir_params.nvec[0];
    std::uint64_t n_1 = all->pir_params.nvec[1];

    std::uint64_t ele_index = 0;
    std::uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    std::cout << "Main: element index = " << ele_index << " from [0, "
              << all->pir_params.ele_num - 1 << "]" << std::endl;
    PirQuery query = client.generate_query(index);

    auto expander = multiplication_utils::QueryExpander::Create(all->encryption_params);

    auto expanded_query_dim_0 = expander->expand_query(query[0], n_0, galois_keys);
    auto expanded_query_dim_1 = expander->expand_query(query[1], n_1, galois_keys);
    // mult it.
}