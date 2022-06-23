//
// test that  ([x, y] * [[a, b], [c, d]]) * [e, f]^T = [x, y] * ([[a, b], [c, d]] * [e, f]^T) in the ciphertext space
// were e, f are both ciphertexts.
//
// in other words, test that e * (a + c) + f * (b + d) = (a*e + b*f) + (c*e + d*f)
//


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

    uint32_t N = 4096;

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

    size_t slot_count = encoder.slot_count();

    vector<uint64_t> coefficients(slot_count, 0ULL);
    vector<uint64_t> coefficients_squared(slot_count, 0ULL);
    for (uint32_t i = 0; i < coefficients.size(); i++) {
        coefficients[i] = (rand() % 256);
        coefficients_squared[i] = coefficients[i] * 2;  //pt*pt
    }

    Plaintext pt;
    encoder.encode(coefficients, pt);
    Ciphertext ct;
    encryptor.encrypt_symmetric(pt, ct);
    std::cout << "Encrypting" << std::endl;//1958182 1860171
    std::cout << enc_params.plain_modulus().value() << std::endl;//1958182 1


    Plaintext pt_doubled;
    encoder.encode(coefficients_squared, pt_doubled);

    auto context_data = context.last_context_data();
    auto parms_id = context.last_parms_id();

    Ciphertext x_times_plain1;
    Ciphertext x_times_plain2;

    evaluator.multiply_plain(ct, pt, x_times_plain1);
    evaluator.multiply_plain(ct, pt, x_times_plain2);

    Ciphertext twox_times_twoplain1;

    //(ct*pt) + (ct*pt)
    evaluator.add(x_times_plain1, x_times_plain2, twox_times_twoplain1);


    Ciphertext x_times_x;

    Ciphertext ct_plus_ct;
    evaluator.add(ct, ct, ct_plus_ct);

    //pt*(ct+ct)
    evaluator.multiply_plain(ct_plus_ct, pt, x_times_x);


    Ciphertext trivial_result;
    evaluator.sub(x_times_x, twox_times_twoplain1, trivial_result);



    Plaintext pt2;
    decryptor.decrypt(trivial_result, pt2);
    assert(pt2.is_zero());

    Plaintext pt3;
    decryptor.decrypt(twox_times_twoplain1, pt3);

    Plaintext pt4;
    decryptor.decrypt(x_times_x, pt4);
    assert(pt3 == pt4);
    std::cout <<trivial_result.coeff_modulus_size()<<std::endl;
    assert(trivial_result.is_transparent());

    std::cout << "Worked" << std::endl;

    return 0;
}

