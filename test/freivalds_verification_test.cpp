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
#include "FreivaldsVector.hpp"

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


    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();

    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            auto val = gen->generate() % 256;
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

    // Measure database setup
    server.set_database(move(db), number_of_items, size_per_item);

    // Choose an index of an element in the DB
    uint64_t ele_index = gen->generate() % number_of_items; // element in DB at random position
    uint64_t index = client.get_fv_index(ele_index);   // index of FV plaintext
    uint64_t offset = client.get_fv_offset(ele_index); // offset in FV plaintext
    cout << "Main: element index = " << ele_index << " from [0, " << number_of_items -1 << "]" << endl;
    cout << "Main: FV index = " << index << ", FV offset = " << offset << endl;

    auto reg_query = client.generate_query(index);
    cout << "Main: query generated" << endl;

    //store query for multiple server computation
    auto expanded_query = server.get_expanded_query_single_dim(0, reg_query, 0);



    std::vector<std::vector<std::uint64_t>> db_unencoded(pir_params.nvec[0]*pir_params.nvec[1]);
    server.db_to_vec(db_unencoded);
    FreivaldsVector f(enc_params, pir_params);
    f.multiply_with_db(db_unencoded);

    server.preprocess_database();
    cout << "Main: database pre processed " << endl;

    f.multiply_with_query(0, expanded_query);

    auto answer = server.generate_reply_one_dim(reg_query, 0);

    // do computation at each client
    for(int i=0; i< pir_params.nvec[1]; i++) {
        stringstream s;
        answer[i].save(s);
        f.multiply_with_reply(0, i, s.str());
    }
    return 0;
}
