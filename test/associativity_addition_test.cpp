#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <seal/seal.h>

using namespace std::chrono;
using namespace std;
using namespace seal;

int associativity_addition_test(int, char*[]) {

    uint64_t number_of_items = 2048;
    uint64_t size_per_item = 288; // in bytes
    uint32_t N = 8192;

    // Recommended values: (logt, d) = (12, 2) or (8, 1).
    uint32_t logt = 20;

    EncryptionParameters enc_params(scheme_type::bgv);

    // Generates all parameters

    cout << "Main: Generating SEAL parameters" << endl;
    gen_encryption_params(N, logt, enc_params);

    cout << "Main: Verifying SEAL parameters" << endl;
    verify_encryption_params(enc_params);
    cout << "Main: SEAL parameters are good" << endl;

    SEALContext context(enc_params, true);
    KeyGenerator keygen(context);

    SecretKey secret_key = keygen.secret_key();
    Encryptor encryptor(context, secret_key);
    Decryptor decryptor(context, secret_key);
    Evaluator evaluator(context);
    BatchEncoder encoder(context);
    logt = floor(log2(enc_params.plain_modulus().value()));

    uint32_t plain_modulus = enc_params.plain_modulus().value();

    size_t slot_count = encoder.slot_count();

    vector<uint64_t> coefficients(slot_count, 0ULL);
    vector<uint64_t> coefficients_doubled(slot_count, 0ULL);
    for (uint32_t i = 0; i < coefficients.size(); i++) {
        coefficients[i] = (rand() % 5);
        coefficients_doubled[i] = coefficients[i]*2;  //2*pt
    }

    Plaintext pt;
    encoder.encode(coefficients, pt);
    Ciphertext ct;
    encryptor.encrypt_symmetric(pt, ct);
    std::cout << "Encrypting" << std::endl;

    Plaintext pt_doubled;
    encoder.encode(coefficients_doubled, pt_doubled);

    auto context_data = context.last_context_data();
    auto parms_id = context.last_parms_id();

    Ciphertext x_plus_plain1;
    Ciphertext x_plus_plain2;

    evaluator.add_plain(ct, pt, x_plus_plain1);
    evaluator.add_plain(ct, pt, x_plus_plain2);

    Ciphertext twox_plus_twoplain1;

    evaluator.add(x_plus_plain1, x_plus_plain2, twox_plus_twoplain1);// ((x+p)+(x+p))


    Ciphertext x_plus_x;
    evaluator.add(ct, ct, x_plus_x);

    Ciphertext twox_plus_twoplain2;

    evaluator.add_plain(x_plus_x, pt_doubled, twox_plus_twoplain2); //((x+x)+(2*p))

    Ciphertext trivial_result;
    evaluator.sub(twox_plus_twoplain1, twox_plus_twoplain2, trivial_result);



    Plaintext pt2;
    decryptor.decrypt(trivial_result, pt2);
    assert(pt2.is_zero());

    Plaintext pt3;
    decryptor.decrypt(twox_plus_twoplain1, pt3);

    Plaintext pt4;
    decryptor.decrypt(twox_plus_twoplain2, pt4);
    assert(pt3 == pt4);

    assert(trivial_result.is_transparent());

    std::cout << "Worked" << std::endl;

    return 0;
}
