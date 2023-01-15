#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include <seal/seal.h>
#include <chrono>
#include <memory>
#include <random>
#include <cstdint>
#include <cstddef>
#include "concurrency/concurrency.h"

using namespace std::chrono;
using namespace std;
using namespace seal;

bool verify_params(int argc, char *const *argv) {
    if(argc != 3 || std::stoi(argv[1]) < 1 || std::stoi(argv[2]) <= 0)
    {
        cout << "Usage: " << argv[0] << " <num_threads>" << " <num_queries>" << endl;
        return false;
    }
    return true;
}

struct indice{
    uint64_t ele_index;
    uint64_t offset;
};

int main(int argc, char *argv[]) {
    auto all_good = verify_params(argc, argv);
    if (!all_good)
    {
        return -1;
    }

    uint32_t num_threads = std::stoi(argv[1]);

    uint32_t  num_queries = std::stoi(argv[2]);


    uint64_t number_of_items = 1 << 16;
    uint64_t size_per_item = 256; // in bytes
    uint32_t N = 4096;

    // Recommended values: (logt, d) = (20, 2).
    uint32_t logt = 20;
    uint32_t d = 2;
    bool use_symmetric = true; // use symmetric encryption instead of public key (recommended for smaller query)
    bool use_batching = true; // pack as many elements as possible into a BFV plaintext (recommended)
    bool use_recursive_mod_switching = false;

    EncryptionParameters enc_params(scheme_type::bgv);
    PirParams pir_params;

    // Generates all parameters

    cout << "Main: Generating SEAL parameters" << endl;
    gen_encryption_params(N, logt, enc_params);

    cout << "Main: Verifying SEAL parameters" << endl;
    verify_encryption_params(enc_params);
    cout << "Main: SEAL parameters are good" << endl;

    cout << "Main: Generating PIR parameters" << endl;
    gen_pir_params(number_of_items, size_per_item, d, enc_params, pir_params, use_symmetric, use_batching, use_recursive_mod_switching);


    print_seal_params(enc_params);
    print_pir_params(pir_params);

    PIRServer server(enc_params, pir_params);

    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();

    // Create test database
    auto db(make_unique<uint8_t[]>(number_of_items * size_per_item));
    auto db_copy(make_unique<uint8_t[]>(number_of_items * size_per_item));
    for (uint64_t i = 0; i < number_of_items; i++) {
        for (uint64_t j = 0; j < size_per_item; j++) {
            auto val = gen->generate() % 256;
            db.get()[(i * size_per_item) + j] = val;
            db_copy.get()[(i * size_per_item) + j] = val;
        }
    }
    server.set_database(move(db), number_of_items, size_per_item);
    server.preprocess_database();

    //create clients and queries
    std::vector<PIRClient> clients;
    std::vector<PirQuery> queries;
    std::vector<PirReply> answers(num_queries);
    std::vector<indice> indices(num_queries);

    for (int i=0; i<num_queries; i++) {
        clients.push_back(PIRClient(enc_params, pir_params));
        GaloisKeys galois_keys = clients[i].generate_galois_keys();
        server.set_galois_key(i, galois_keys);
        uint64_t ele_index = gen->generate() % number_of_items; // element in DB at random position
        uint64_t index = clients[i].get_fv_index(ele_index);   // index of FV plaintext
        uint64_t offset = clients[i].get_fv_offset(ele_index); // offset in FV plaintext
        queries.push_back(clients[i].generate_query(index));
        indices[i].ele_index = ele_index;
        indices[i].offset = offset;

    }


    concurrency::threadpool* pool = new concurrency::threadpool(num_threads);
    concurrency::safelatch latch(num_queries);
    auto time_pool_s = high_resolution_clock::now();
    for (int j=0; j<num_queries; j++) {

        pool->submit(
                std::move(concurrency::Task{
                        .f = [&, j]() {
                            answers[j] = server.generate_reply(queries[j], j);
                            latch.count_down();
                        },
                        .wg = nullptr,
                        .name = "server:generate_reply",
                })
        );

    }
    latch.wait();
    auto time_pool_e = high_resolution_clock::now();
    auto time_pool_us =
            duration_cast<microseconds>(time_pool_e - time_pool_s).count();

    clog << "Main: pool query processing time: " << time_pool_us / 1000
         << " ms on "<<num_queries << " queries and "<< num_threads << " threads" << endl;
    delete pool;


    for (int i=0; i<num_queries; i++) {
        vector<uint8_t> elems = clients[i].decode_reply(answers[i], indices[i].offset);
        assert(elems.size() == size_per_item);
        bool failed = false;

        // Check that we retrieved the correct element
        for (uint32_t k = 0; k < size_per_item; k++) {
            if (elems[k] != db_copy.get()[(indices[i].ele_index * size_per_item) + k]) {
                cout << "Main: elems " << (int)elems[k] << ", db "
                     << (int) db_copy.get()[(indices[i].ele_index * size_per_item) + k] << endl;
                cout << "Main: PIR result wrong at " << k <<  endl;
                failed = true;
            }
        }
        if(failed){
            return -1;
        }


    }

    return 0;
}