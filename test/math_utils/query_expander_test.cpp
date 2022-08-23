#include <query_expander.hpp>
#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include "../test_utils.hpp"

int query_expander_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);
    auto expander = multiplication_utils::QueryExpander::Create(all->encryption_params);
    // i want to get the expansion that would give me the correct result.  i might need to copy some code from sealpir
    auto number_of_items = 4096;
    gen_pir_params(number_of_items, 288, 2, all->encryption_params, all->pir_params);
    PIRClient client(all->encryption_params, all->pir_params);
    auto galois_keys = client.generate_galois_keys();

    random_device rd;
    // Choose an index of an element in the DB
    uint64_t ele_index =
           rd() % number_of_items; // element in DB at random position
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
    cout << "Main: element index = " << ele_index << " from [0, "
         << number_of_items - 1 << "]" << endl;
    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;

    // Measure query generation

    PirQuery query = client.generate_query(index);

    // Measure query processing (including expansion)

    uint64_t n_i = all->pir_params.nvec[0];
    std::vector<seal::Ciphertext> expanded_query;
    expander->ExpandQuery(query[0], n_i, galois_keys, expanded_query);

    assert(expanded_query.size() == n_i);

    cout << "Main: checking expansion" << endl;
    for (size_t i = 0; i < expanded_query.size(); i++) {
        seal::Plaintext decryption = client.decrypt(expanded_query.at(i));

        if (decryption.is_zero() && index != i) {
            continue;
        }
        else if (decryption.is_zero()) {
            cout << "Found zero where index should be" << endl;
            return -1;
        }
        else if (std::stoi(decryption.to_string()) != 1) {
            cout << "Query vector at index " << index
                 << " should be 1 but is instead " << decryption.to_string() << endl;
            throw std::runtime_error("Query expansion failed");
            return -1;
        }
        else {
            cout << "Query vector at index " << index << " is "
                 << decryption.to_string() << endl;
            if (decryption.to_string() != "1"){
                throw std::runtime_error("Query expansion failed");
            }
        }
    }
    return 0;
}