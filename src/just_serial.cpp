#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <seal/seal.h>

#include "concurrency/concurrency.h"
#include "matrix.h"

using namespace std::chrono;
using namespace std;
using namespace seal;

// For this test, we need the parameters to be such that the number of
// compressed ciphertexts needed is 1.
int main(int argc, char *argv[]) {

    uint64_t number_of_items = 65536;
    uint64_t size_per_item = 1024; // in bytes
    uint32_t N = 4096;

    // Recommended values: (logt, d) = (12, 2) or (8, 1).
    uint32_t logt = 20;
    uint32_t d = 2;

    uint32_t num_threads = 8;
    uint32_t num_queries = 128;

    EncryptionParameters enc_params(scheme_type::bgv);
    PirParams pir_params;

    // Generates all parameters

    cout << "Main: Generating SEAL parameters" << endl;
    gen_encryption_params(N, logt, enc_params);

    cout << "Main: Verifying SEAL parameters" << endl;
    verify_encryption_params(enc_params);
    cout << "Main: SEAL parameters are good" << endl;

    cout << "Main: Generating PIR parameters" << endl;
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params);

    // gen_params(number_of_items, size_per_item, N, logt, d, enc_params,
    // pir_params);
    print_pir_params(pir_params);

    // Initialize PIR Server
    cout << "Main: Initializing server and client" << endl;
    PIRServer server(enc_params, pir_params);

    // Initialize PIR client....
    PIRClient client(enc_params, pir_params);
    GaloisKeys galois_keys = client.generate_galois_keys();

    // Set galois key for client with id 0
    cout << "Main: Setting Galois keys...";
    server.set_galois_key(0, galois_keys);

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
    auto time_query_s = high_resolution_clock::now();
    PirQuery query = client.generate_query(index);
    auto time_query_e = high_resolution_clock::now();
    auto time_query_us =
            duration_cast<microseconds>(time_query_e - time_query_s).count();
    cout << "Main: query generated" << endl;

    // Measure query processing (including expansion)
    int i = 0;
    uint64_t n_i = pir_params.nvec[0];
//    vector<Ciphertext> expanded_query = server.expand_query(query[0][0], n_i, 0);
    auto time_server_s = high_resolution_clock::now();
    cout << "Main: n_i is................." << n_i << endl;

    while (i < num_queries) {
         server.expand_query(query[0][0], n_i, 0);
        i++;
    }
    auto time_server_e = high_resolution_clock::now();
    auto time_server_us =
            duration_cast<microseconds>(time_server_e - time_server_s).count();
    cout << "Main: query expanded" << endl;


    cout << "Main: expansion time: " << time_server_us / 1000
         << " ms" << endl;

    cout << "Main: avg expansion time: " << (time_server_us / num_queries) / 1000
         << " ms" << endl;

    return 0;
}