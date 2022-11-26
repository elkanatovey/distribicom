#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include "old_src/master.hpp"
#include "worker.hpp"
#include <seal/seal.h>
#include <chrono>
#include <memory>
#include <random>
#include <cstdint>
#include <cstddef>
#include "old_src/FreivaldsVector.hpp"

using namespace std::chrono;
using namespace std;
using namespace seal;

int distribute_query_ser_test(uint64_t num_items, uint64_t item_size, uint32_t degree, uint32_t lt,
                              uint32_t dim);

int distributed_freivalds_verification_test(int argc, char *argv[]) {
//    // sanity check
//    assert(distribute_query_ser_test(16, 288, 4096, 20, 2) == 0);
//
//    // speed check
//    assert(distribute_query_ser_test(1 << 10, 288, 4096, 20, 2) == 0);
//
//    assert(distribute_query_ser_test(1 << 12, 288, 4096, 20, 2) == 0);
    return 0;
}

//int distribute_query_ser_test(uint64_t num_items, uint64_t item_size, uint32_t degree, uint32_t lt, uint32_t dim){
//
//    uint64_t number_of_items = num_items;
//    uint64_t size_per_item = item_size; // in bytes
//    uint32_t N = degree;
//
//    // Recommended values: (logt, d) = (12, 2) or (8, 1).
//    uint32_t logt = lt;
//    uint32_t d = dim;
//
//    bool use_symmetric = true; // use symmetric encryption instead of public key (recommended for smaller query)
//    bool use_batching = true; // pack as many elements as possible into a BFV plaintext (recommended)
//    bool use_recursive_mod_switching = true;
//
//    EncryptionParameters enc_params(scheme_type::bfv);
//    PirParams pir_params;
//
//    // Generates all parameters
//    cout << "Main: Generating SEAL parameters" << endl;
//    gen_encryption_params(N, logt, enc_params);
//
//    cout << "Main: Verifying SEAL parameters" << endl;
//    verify_encryption_params(enc_params);
//    cout << "Main: SEAL parameters are good" << endl;
//
//    cout << "Main: Generating PIR parameters" << endl;
//    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params, use_symmetric,
//                   use_batching, use_recursive_mod_switching);
//
//
//
//    //gen_params(number_of_items, size_per_item, N, logt, d, enc_params, pir_params);
//    print_pir_params(pir_params);
//
//    cout << "Main: Initializing the database (this may take some time) ..." << endl;
//
//    // Create test database
//    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
//
//    // Copy of the database. We use this at the end to make sure we retrieved
//    // the correct element.
//    auto db_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
//
//
//    seal::Blake2xbPRNGFactory factory;
//    auto gen =  factory.create();
//
//    for (uint64_t i = 0; i < number_of_items; i++) {
//        for (uint64_t j = 0; j < size_per_item; j++) {
//            auto val = gen->generate() % 256;
//            db.get()[(i * size_per_item) + j] = val;
//            db_copy.get()[(i * size_per_item) + j] = val;
//        }
//    }
//
//    // Initialize PIR Server
//    cout << "Main: Initializing server and client" << endl;
//    Master server(enc_params, pir_params);
//
//    // Initialize PIR client....
//    PIRClient client(enc_params, pir_params);
//    GaloisKeys galois_keys = client.generate_galois_keys();
//
//    // Set galois key for client with id 0
//    cout << "Main: Setting Galois keys...";
//    stringstream gkey;
//    galois_keys.save(gkey);
//    server.store_galois_key_ser(0, gkey);
//
//    // Measure database setup
//    server.set_database(move(db), number_of_items, size_per_item);
//
//    std::vector<std::vector<std::uint64_t>> db_unencoded(pir_params.nvec[0]*pir_params.nvec[1]);
//    server.db_to_vec(db_unencoded);
//    FreivaldsVector f(enc_params, pir_params);
//    f.multiply_with_db(db_unencoded);
//
//
//    server.preprocess_database();
//    cout << "Main: database pre processed " << endl;
//    auto time_pre_e = high_resolution_clock::now();
//
//    // Choose an index of an element in the DB
//    uint64_t ele_index = gen->generate() % number_of_items; // element in DB at random position
//    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
//    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
//    cout << "Main: element index = " << ele_index << " from [0, " << number_of_items -1 << "]" << endl;
//    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;
//
//    stringstream query;
//    // Measure query generation
//    auto time_query_s = high_resolution_clock::now();
//    client.generate_serialized_query(index, query);
//    auto time_query_e = high_resolution_clock::now();
//    auto time_query_us = duration_cast<microseconds>(time_query_e - time_query_s).count();
//    cout << "Main: query generated" << endl;
//
//    //store query for multiple server computation
//    auto query_string = query.str();
//    server.store_query_ser(0, query_string);
//    stringstream query_copy;
//    query_copy.str(query_string);
//    auto expanded_query = server.get_expanded_query_first_dim_ser(0, query_copy);
//
//
//    f.multiply_with_query(0, expanded_query);
//
//
//
//    // do computation at each client
//    for(int i=0; i< pir_params.nvec[1]; i++) {
//        auto row = server.get_db_row_serialized(i);
//        auto row_len = server.get_row_len();
//        stringstream temp_db_;
//        temp_db_.str(row);
//
//        DistributedQueryContextBucketSerial bucket = server.get_query_bucket_to_compute_serialized(0);
//        Worker clientSideServer(enc_params, pir_params, temp_db_, i, row_len);
//        auto gkeys_ser = server.get_galois_bucket_ser(0);
//
//        // add client galois key - needed for expand
//        stringstream galois_str;
//        for(int j=0;j<gkeys_ser.size();j++){
//            galois_str.str(gkeys_ser[j].galois);
//            clientSideServer.set_one_galois_key_ser(gkeys_ser[j].client_id, galois_str);
//            galois_str.clear();
//        }
//
//        //measure individual client calculation time
//        auto time_client_s = high_resolution_clock::now();
//        stringstream worker_query_stream;
//        stringstream worker_reply_stream;
//
//        // calculate partial reply
//        // for storing individual answers of single client in the test - dont need totrack id here as only client 0
//        // answers being given
//        vector<std::string> partial_answers;
//        for(auto & query_context : bucket){
//            worker_query_stream.str(query_context.query);
//            clientSideServer.process_query_at_client_ser(worker_query_stream, worker_reply_stream, query_context
//            .client_id, false);
//            worker_query_stream.clear();
//            partial_answers.push_back(worker_reply_stream.str());
//            worker_reply_stream.clear();
//        }
//
//
//        auto time_client_e = high_resolution_clock::now();
//        auto time_client_us = duration_cast<microseconds>(time_client_e - time_client_s).count();
//        cout << "Main: Worker "<< i<<" reply generation time: " << time_client_us / 1000 << " ms ............................."
//        << endl;
//
//        stringstream partial_reply_stream;
//        for(auto & answer : partial_answers){
//            partial_reply_stream << answer;
//            f.multiply_with_reply(0, server.get_row_id(i), answer);
//            partial_reply_stream.clear();
//        }
//    }
//    return 0;
//}
