#include "pir.hpp"
#include "pir_client.hpp"

#include "master.hpp"
#include "FreivaldsVector.hpp"

#include <seal/seal.h>
#include "test_utils.hpp"

using namespace std::chrono;
using namespace std;
using namespace seal;

int freivalds_verification_test(TestUtils::SetupConfigs t, bool random_freivalds_vec);

int main() {

    auto test_configs = TestUtils::SetupConfigs{
            .encryption_params_configs = TestUtils::DEFAULT_ENCRYPTION_PARAMS_CONFIGS,
            .pir_params_configs= {
                    .number_of_items = 4096 * 3,
                    .size_per_item = 2,
                    .dimensions = 2,
                    .use_symmetric = true,
                    .use_batching = true,
                    .use_recursive_mod_switching = false,
            },
    };

    // sanity check
    assert(freivalds_verification_test(test_configs, false) == 0);

    test_configs.pir_params_configs.number_of_items = 1 << 10;
    test_configs.pir_params_configs.size_per_item = 288;
    // speed check
    assert(freivalds_verification_test(test_configs, false) == 0);

    test_configs.pir_params_configs.number_of_items = 1 << 12;
    assert(freivalds_verification_test(test_configs, false) == 0);

    test_configs.pir_params_configs.number_of_items = 1 << 16;
    test_configs.pir_params_configs.size_per_item = 1024;
    assert(freivalds_verification_test(test_configs, false) == 0);
}

int freivalds_verification_test(TestUtils::SetupConfigs t, bool random_freivalds_vec) {
    auto num_items = t.pir_params_configs.number_of_items;
    auto size_per_item = t.pir_params_configs.size_per_item;
    auto all = TestUtils::setup(t);


    print_pir_params(all->pir_params);

    cout << "Main: Initializing the database (this may take some time) ..." << endl;

    // Create test database
    auto db(make_unique<uint8_t[]>(num_items * size_per_item));

    // Copy of the database. We use this at the end to make sure we retrieved
    // the correct element.
    auto db_copy(make_unique<uint8_t[]>(num_items * size_per_item));


    seal::Blake2xbPRNGFactory factory;
    auto gen = factory.create();

    for (uint64_t i = 0; i < num_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            auto val = gen->generate() % 255;
            db.get()[(i * size_per_item) + j] = val;
            db_copy.get()[(i * size_per_item) + j] = val;
        }
    }

    // Initialize PIR Server
    cout << "Main: Initializing server and client" << endl;
    MasterServer server(all->encryption_params, all->pir_params);

    // Initialize PIR client....
    PIRClient client(all->encryption_params, all->pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();

    // Set galois key for client with id 0
    cout << "Main: Setting Galois keys...";
    server.set_galois_key(0, galois_keys);

    // Measure database setup
    server.set_database(move(db), num_items, size_per_item);

    // Choose an index of an element in the DB
    uint64_t ele_index = gen->generate() % num_items; // element in DB at random position
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
    cout << "Main: element index = " << ele_index << " from [0, " << num_items - 1 << "]" << endl;
    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;

    auto reg_query = client.generate_query(index);
    cout << "Main: query generated" << endl;

    //store query for multiple server computation
    auto expanded_query = server.get_expanded_query_single_dim(0, reg_query, 0);
//    auto db_pointer =server.get_db();

    auto db_fake_enc = server.get_fake_enc_db(expanded_query[0]);
    auto mSharedPtr = std::make_shared<std::vector<seal::Ciphertext> >(std::move(db_fake_enc));

    function<uint32_t(void)> g;
    if (random_freivalds_vec) {
        g = [&gen]() { return gen->generate() % 2; };
    }
    else {
        g = []() { return 1; };
    }
    FreivaldsVector f(all->encryption_params, all->pir_params, g);
//    f.mult_rand_vec_by_db(db_pointer);
    f.mult_rand_vec_by_db(mSharedPtr);

    server.preprocess_database();
    cout << "Main: database pre processed " << endl;

    f.multiply_with_query(0, expanded_query);

    auto answer = server.generate_reply_one_dim_enc(reg_query, 0, mSharedPtr.get());
//    auto answer = server.generate_reply_one_dim(reg_query, 0);
    seal::Ciphertext response;
    // do computation at each client
    f.multiply_with_reply(0, answer, response);
    auto diff = client.decrypt(response);

    return 0;
}
