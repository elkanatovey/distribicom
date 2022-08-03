#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include "master.hpp"
#include "worker.hpp"
#include <seal/seal.h>
#include <chrono>
#include <memory>
#include <random>
#include <cstdint>
#include <cstddef>
#include "test_utils.cpp"

using namespace std::chrono;
using namespace std;
using namespace seal;

int distribute_query_test(TestUtils::SetupConfigs);

int main() {
    auto test_configs = TestUtils::SetupConfigs{
            .encryption_params_configs = {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096,
                    .log_coefficient_modulus = 20
            },
            .pir_params_configs= {
                    .number_of_items = 16,
                    .size_per_item = 288,
                    .dimensions = 2,
                    .use_symmetric = true,
                    .use_batching = true,
                    .use_recursive_mod_switching = true,
            },
    };
    // sanity check
    assert(distribute_query_test(test_configs) == 0);

    // speed check
    test_configs.pir_params_configs.number_of_items = 1 << 10;
    assert(distribute_query_test(test_configs) == 0);

    test_configs.pir_params_configs.number_of_items = 1 << 12;
    assert(distribute_query_test(test_configs) == 0);

}

int distribute_query_test(TestUtils::SetupConfigs t) {
    auto all = TestUtils::setup(t);

    print_pir_params(all->pir_params);

    auto number_of_items = t.pir_params_configs.number_of_items;
    auto size_per_item = t.pir_params_configs.size_per_item;

    cout << "Main: Initializing the database (this may take some time) ..." << endl;

    // Create test database
    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));

    // Copy of the database. We use this at the end to make sure we retrieved
    // the correct element.
    auto db_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));

    seal::Blake2xbPRNGFactory factory;
    auto gen = factory.create();

    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            auto val = gen->generate() % 256;
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
    auto time_pre_s = high_resolution_clock::now();
    server.set_database(move(db), number_of_items, size_per_item);
    server.generate_dbase_partition(); //@todo pretty this up
    server.preprocess_database();
    cout << "Main: database pre processed " << endl;
    auto time_pre_e = high_resolution_clock::now();
    auto time_pre_us = duration_cast<microseconds>(time_pre_e - time_pre_s).count();

//    auto s = server.get_db_row_serialized(0);
//    auto z = deserialize_plaintexts(server.get_row_len(), s, server.get_ptext_len());
//    auto z1 =  server.get_db_row(0);

    // Choose an index of an element in the DB
    uint64_t ele_index = gen->generate() % number_of_items; // element in DB at random position
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
    cout << "Main: element index = " << ele_index << " from [0, " << number_of_items - 1 << "]" << endl;
    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;

    // Measure query generation
    auto time_query_s = high_resolution_clock::now();
    PirQuery query = client.generate_query(index);
    auto time_query_e = high_resolution_clock::now();
    auto time_query_us = duration_cast<microseconds>(time_query_e - time_query_s).count();
    cout << "Main: query generated" << endl;
    //To marshall query to send over the network, you can use serialize/deserialize:
    //std::string query_ser = serialize_query(query);
    //PirQuery query2 = deserialize_query(d, 1, query_ser, CIPHER_SIZE

    //store query for multiple server computation
    server.store_query(query, 0);

    // do computation at each client
    for (int i = 0; i < all->pir_params.nvec[1]; i++) {
        DistributedQueryContextBucket bucket = server.get_query_bucket_to_compute(0);
        auto row = server.get_db_row(i);
        auto dbShard = make_unique<vector<seal::Plaintext>>();
        dbShard->reserve(row.size());
        for (auto &i: row) {
            dbShard->push_back(move(i));
        }
        ClientSideServer clientSideServer(all->encryption_params, all->pir_params, std::move(dbShard), i);
        clientSideServer.set_galois_key(0, galois_keys);
        //measure individual client calculation time
        auto time_client_s = high_resolution_clock::now();
        auto clientReplies = clientSideServer.processQueryBucketAtClient(bucket);
        auto time_client_e = high_resolution_clock::now();
        auto time_client_us = duration_cast<microseconds>(time_client_e - time_client_s).count();
        cout << "Main: ClientSideServer " << i << " reply generation time: " << time_client_us / 1000
             << " ms ............................."
             << endl;
        server.processQueriesAtServer(clientReplies);
    }

    auto reply = server.generateFinalReply(0);


    // Measure query processing (including expansion) in normal server
    auto time_server_s = high_resolution_clock::now();
    PirReply repl1y = server.generate_reply(query, 0);
    auto time_server_e = high_resolution_clock::now();
    auto time_server_us = duration_cast<microseconds>(time_server_e - time_server_s).count();

    // Measure response extraction
    auto time_decode_s = chrono::high_resolution_clock::now();
    vector<uint8_t> elems = client.decode_reply(reply, offset);
    auto time_decode_e = chrono::high_resolution_clock::now();
    auto time_decode_us = duration_cast<microseconds>(time_decode_e - time_decode_s).count();

    assert(elems.size() == size_per_item);

    bool failed = false;
    // Convert from FV plaintext (polynomial) to database element at the client;

    // Check that we retrieved the correct element
    for (uint32_t i = 0; i < size_per_item; i++) {
        if (elems[i] != db_copy.get()[(ele_index * size_per_item) + i]) {
            cout << "Main: elems " << (int) elems[i] << ", db "
                 << (int) db_copy.get()[(ele_index * size_per_item) + i] << endl;
            cout << "Main: PIR result wrong at " << i << endl;
            failed = true;
        }
    }
    if (failed) {
        return -1;
    }


    // Output results
    cout << "Main: PIR result correct!" << endl;
    cout << "Main: PIRServer pre-processing time: " << time_pre_us / 1000 << " ms" << endl;
    cout << "Main: PIRClient query generation time: " << time_query_us / 1000 << " ms" << endl;
    cout << "Main: PIRServer reply generation time: " << time_server_us / 1000 << " ms"
         << endl;
    cout << "Main: PIRClient answer decode time: " << time_decode_us / 1000 << " ms" << endl;
    cout << "Main: Reply num ciphertexts: " << reply.size() << endl;

    return 0;
}
