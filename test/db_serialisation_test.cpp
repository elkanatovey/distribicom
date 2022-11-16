#include "pir.hpp"

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

int db_serialisation_test1(uint64_t num_items, uint64_t item_size, uint32_t degree, uint32_t lt,
                           uint32_t dim);

int db_serialisation_test(int argc, char *argv[]) {
    // sanity check
    //    assert(db_serialisation_test1(16, 288, 4096, 20, 2) == 0);

    // speed check
    assert(db_serialisation_test1(1 << 10, 288, 4096, 20, 2) == 0);

    assert(db_serialisation_test1(1 << 12, 288, 4096, 20, 2) == 0);
    return 0;

}

int db_serialisation_test1(uint64_t num_items, uint64_t item_size, uint32_t degree, uint32_t lt, uint32_t dim) {

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

    auto context_ = make_shared<SEALContext>(enc_params, true);

    print_pir_params(pir_params);

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
    Master server(enc_params, pir_params);

    server.set_database(move(db), number_of_items, size_per_item);
    server.generate_dbase_partition();



    // do computation at each client
    for (int i = 1; i < pir_params.nvec[1]; i++) {
        //get serialized partition
        auto row_serialised = server.get_db_row_serialized(i);
        auto row_len = server.get_row_len();

        stringstream temp_db_;
        temp_db_.str(row_serialised);
        auto db_ = make_shared<vector<seal::Plaintext>>();
        deserialize_db(*context_, temp_db_, row_len, db_);

        // get nonserial row
        auto row_not_serial = server.get_db_row(i);
        auto db_shard_nonserialized = make_unique<vector<seal::Plaintext>>();
        db_shard_nonserialized->reserve(row_serialised.size());
        for (auto &j: row_not_serial) {
            db_shard_nonserialized->push_back(move(j));
        }

        // compare
        for (std::uint64_t k = 0; k < db_shard_nonserialized->size(); k++) {
            std::cout << "plaintext number " << k << std::endl;
            assert((*db_shard_nonserialized)[k].to_string() == (*db_)[k].to_string());
        }

    }

    bool failed = false;

    if (failed) {
        return -1;
    }

    // Output results
    cout << "Main: Deserialization result correct!" << endl;

    return 0;
}
