#include <query_expander.hpp>
#include "pir.hpp"
#include "pir_client.hpp"
#include "../test_utils.hpp"

// Throws on failure:
void correct_expansion_test(TestUtils::SetupConfigs);

void expanding_full_dimension_query(TestUtils::SetupConfigs);

int query_expander_test(int, char *[]) {
    auto dcfs = TestUtils::SetupConfigs{
            .encryption_params_configs = {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096,
                    .log_coefficient_modulus = 20,
            },
            .pir_params_configs = {
                    .number_of_items = 2048,
                    .size_per_item = 288,
                    .dimensions= 1,
                    .use_symmetric = false,
                    .use_batching = true,
                    .use_recursive_mod_switching = true,
            },
    };
    correct_expansion_test(dcfs);
    return 0;
}

void correct_expansion_test(TestUtils::SetupConfigs dcfs) {
    auto all = TestUtils::setup(dcfs);

    // Initialize PIR client....
    PIRClient client(all->encryption_params, all->pir_params);
    seal::GaloisKeys galois_keys = client.generate_galois_keys();


    auto expander = multiplication_utils::QueryExpander::Create(all->encryption_params);
    for (std::uint64_t k = 0; k < all->pir_params.ele_num; ++k) {
        uint64_t ele_index = k;

        uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
        uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
        std::cout << "Main: element index = " << ele_index << " from [0, "
                  << all->pir_params.ele_num - 1 << "]" << std::endl;
        std::cout << "Main: FV index = " << index << ", FV offset = " << offset << std::endl;

        // Measure query generation

        PirQuery query = client.generate_query(index);
        std::cout << "Main: query generated" << std::endl;

        uint64_t n_i = all->pir_params.nvec[0];
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

void expanding_full_dimension_query(TestUtils::SetupConfigs) {
// need to expand a single query, and multiply it with some DB.
// basically mult once from left and once from the right.
// and expect a specific element.

}