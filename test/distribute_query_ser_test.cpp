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

using namespace std::chrono;
using namespace std;
using namespace seal;

int distribute_query_ser_test(uint64_t num_items, uint64_t item_size, uint32_t degree, uint32_t lt,
                           uint32_t dim);

int main(int argc, char *argv[]) {
    // sanity check
    assert(distribute_query_ser_test(16, 288, 4096, 20, 2) == 0);

    // speed check
    assert(distribute_query_ser_test(1 << 10, 288, 4096, 20, 2) == 0);

    assert(distribute_query_ser_test(1 << 12, 288, 4096, 20, 2) == 0);

}

int distribute_query_ser_test(uint64_t num_items, uint64_t item_size, uint32_t degree, uint32_t lt, uint32_t dim){

    uint64_t number_of_items = num_items;
    uint64_t size_per_item = item_size; // in bytes
    uint32_t N = degree;

    // Recommended values: (logt, d) = (12, 2) or (8, 1).
    uint32_t logt = lt;
    uint32_t d = dim;

    bool use_symmetric = true; // use symmetric encryption instead of public key (recommended for smaller query)
    bool use_batching = true; // pack as many elements as possible into a BFV plaintext (recommended)
    bool use_recursive_mod_switching = true;

    EncryptionParameters enc_params(scheme_type::bfv);
    PirParams pir_params;

    // Generates all parameters
    cout << "Main: Generating SEAL parameters" << endl;
    gen_encryption_params(N, logt, enc_params);

    cout << "Main: Verifying SEAL parameters" << endl;
    verify_encryption_params(enc_params);
    cout << "Main: SEAL parameters are good" << endl;

    cout << "Main: Generating PIR parameters" << endl;
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params, use_symmetric,
                   use_batching, use_recursive_mod_switching);



    //gen_params(number_of_items, size_per_item, N, logt, d, enc_params, pir_params);
    print_pir_params(pir_params);

    cout << "Main: Initializing the database (this may take some time) ..." << endl;

    // Create test database
    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));

    // Copy of the database. We use this at the end to make sure we retrieved
    // the correct element.
    auto db_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));

    random_device rd;
    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            auto val = rd() % 256;
            db.get()[(i * size_per_item) + j] = val;
            db_copy.get()[(i * size_per_item) + j] = val;
        }
    }

    // Initialize PIR Server
    cout << "Main: Initializing server and client" << endl;
    MasterServer server(enc_params, pir_params);

    // Initialize PIR client....
    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();

    // Set galois key for client with id 0
    cout << "Main: Setting Galois keys...";
    server.set_galois_key(0, galois_keys);
    stringstream gkey;
    galois_keys.save(gkey);
    std::string s = gkey.str();
    server.store_galois_key_ser(0, s);

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
    uint64_t ele_index = rd() % number_of_items; // element in DB at random position
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
    cout << "Main: element index = " << ele_index << " from [0, " << number_of_items -1 << "]" << endl;
    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;

    stringstream query;
    // Measure query generation
    auto time_query_s = high_resolution_clock::now();
    client.generate_serialized_query(index, query);
    auto time_query_e = high_resolution_clock::now();
    auto time_query_us = duration_cast<microseconds>(time_query_e - time_query_s).count();
    cout << "Main: query generated" << endl;

    //store query for multiple server computation
    server.store_query_ser(0, query.str());

    // do computation at each client
    for(int i=0; i< pir_params.nvec[1]; i++) {
        DistributedQueryContextBucketSerial bucket = server.get_query_bucket_to_compute_serialized(0);
        auto row = server.get_db_row_serialized(i);
        auto row_len = server.get_row_len();

        ClientSideServer clientSideServer(enc_params, pir_params, row, i, row_len);

        //measure individual client calculation time
        auto time_client_s = high_resolution_clock::now();
//@todo continue serialization
        auto clientReplies = clientSideServer.processQueryBucketAtClient(bucket);
        auto time_client_e = high_resolution_clock::now();
        auto time_client_us = duration_cast<microseconds>(time_client_e - time_client_s).count();
        cout << "Main: ClientSideServer "<< i<<" reply generation time: " << time_client_us / 1000 << " ms ............................."
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
            cout << "Main: elems " << (int)elems[i] << ", db "
                 << (int) db_copy.get()[(ele_index * size_per_item) + i] << endl;
            cout << "Main: PIR result wrong at " << i <<  endl;
            failed = true;
        }
    }
    if(failed){
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
