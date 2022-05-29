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

int main(int argc, char *argv[]) {

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
    vector<uint64_t> coefficients_squared(slot_count, 0ULL);
    for (uint32_t i = 0; i < coefficients.size(); i++) {
        coefficients[i] = (rand() % 5);
        coefficients_squared[i] = coefficients[i] * coefficients[i];  //pt*pt
    }

    Plaintext pt;
    encoder.encode(coefficients, pt);
    Ciphertext ct;
    encryptor.encrypt_symmetric(pt, ct);
    std::cout << "Encrypting" << std::endl;

    Plaintext pt_doubled;
    encoder.encode(coefficients_squared, pt_doubled);

    auto context_data = context.last_context_data();
    auto parms_id = context.last_parms_id();

    Ciphertext x_times_plain1;
    Ciphertext x_times_plain2;

    evaluator.multiply_plain(ct, pt, x_times_plain1);
    evaluator.multiply_plain(ct, pt, x_times_plain2);

    Ciphertext twox_times_twoplain1;

    evaluator.multiply(x_times_plain1, x_times_plain2, twox_times_twoplain1);


    Ciphertext x_times_x;
    evaluator.multiply(ct, ct, x_times_x);

    Ciphertext twox_times_twoplain2;

    evaluator.multiply_plain(x_times_x, pt_doubled, twox_times_twoplain2);

    Ciphertext trivial_result;
    evaluator.sub(twox_times_twoplain1, twox_times_twoplain2, trivial_result);



    Plaintext pt2;
    decryptor.decrypt(trivial_result, pt2);
    assert(pt2.is_zero());

    Plaintext pt3;
    decryptor.decrypt(twox_times_twoplain1, pt3);

    Plaintext pt4;
    decryptor.decrypt(twox_times_twoplain2, pt4);
    assert(pt3 == pt4);

    assert(trivial_result.is_transparent());

    std::cout << "Worked" << std::endl;

    return 0;
}
