//
// test that  ([x, y] * [[a, b], [c, d]]) * [e, f]^T = [x, y] * ([[a, b], [c, d]] * [e, f]^T) in the ciphertext space
// were e, f are both ciphertexts and x, y are both set to 1.
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


    cout << "Main: SEAL parameters generated" << endl;

    cout << "Main: generating matrices" << endl;
    vector<uint64_t> a_arr(slot_count, 0ULL);
    vector<uint64_t> b_arr(slot_count, 0ULL);
    vector<uint64_t> c_arr(slot_count, 0ULL);
    vector<uint64_t> d_arr(slot_count, 0ULL);
    vector<uint64_t> e_arr(slot_count, 0ULL);
    vector<uint64_t> f_arr(slot_count, 0ULL);

    for (uint32_t i = 0; i < a_arr.size(); i++) {
        a_arr[i] = (rand() % 256);
        b_arr[i] = (rand() % 256);
        c_arr[i] = (rand() % 256);
        d_arr[i] = (rand() % 256);
        e_arr[i] = (rand() % 256);
        f_arr[i] = (rand() % 256);
    }

    Plaintext a;
    Plaintext b;
    Plaintext c;
    Plaintext d;
    Plaintext e;
    Plaintext f;
    encoder.encode(a_arr, a);
    encoder.encode(b_arr, b);
    encoder.encode(c_arr, c);
    encoder.encode(d_arr, d);
    encoder.encode(e_arr, e);
    encoder.encode(f_arr, f);

    Ciphertext e_encrypted;
    Ciphertext f_encrypted;

    encryptor.encrypt_symmetric(e, e_encrypted);
    encryptor.encrypt_symmetric(f, f_encrypted);
    std::cout << "Encrypting e f" << std::endl;//1958182 1860171
    std::cout << "Plain Modulus:"<<enc_params.plain_modulus().value() << std::endl;

    std::cout << "calculating e * (a + c) + f * (b + d)"<< std::endl;

    // (a + c) and (b + d)
    vector<uint64_t> a_plus_c_arr(slot_count, 0ULL);
    vector<uint64_t> b_plus_d_arr(slot_count, 0ULL);
    for (uint32_t i = 0; i < a_arr.size(); i++) { //@todo add modulo
        a_plus_c_arr[i] = a_arr[i] + c_arr[i];
        b_plus_d_arr[i] = b_arr[i] + d_arr[i];
    }

    Plaintext a_plus_c;
    Plaintext b_plus_d;
    encoder.encode(a_plus_c_arr, a_plus_c);
    encoder.encode(b_plus_d_arr, b_plus_d);

    Ciphertext e_times_a_plus_c; // e * (a + c)
    Ciphertext f_times_b_plus_d; // f * (b + d)
    evaluator.multiply_plain(e_encrypted, a_plus_c, e_times_a_plus_c);
    evaluator.multiply_plain(f_encrypted, b_plus_d, f_times_b_plus_d);

    Ciphertext e_times_a_plus_c_plus_f_times_b_plus_d; // e * (a + c) + f * (b + d)
    evaluator.add(e_times_a_plus_c, f_times_b_plus_d, e_times_a_plus_c_plus_f_times_b_plus_d);

    std::cout << "finished e * (a + c) + f * (b + d)"<< std::endl;//1958182 1


    std::cout << "calculating (a*e + b*f) + (c*e + d*f)"<< std::endl;//1958182 1
    Ciphertext a_times_e; // a * e
    Ciphertext b_times_f; // b * f
    Ciphertext c_times_e; // c * e
    Ciphertext d_times_f; // d * f
    evaluator.multiply_plain(e_encrypted, a, a_times_e);
    evaluator.multiply_plain(f_encrypted, b, b_times_f);
    evaluator.multiply_plain(e_encrypted, c, c_times_e);
    evaluator.multiply_plain(f_encrypted, d, d_times_f);

    Ciphertext ae_plus_bf; // (a*e + b*f)
    Ciphertext ce_plus_df; // (c*e + d*f)
    evaluator.add(a_times_e, b_times_f, ae_plus_bf);
    evaluator.add(c_times_e, d_times_f, ce_plus_df);

    Ciphertext ae_plus_bf_plus_ce_plus_df; // (a*e + b*f) + (c*e + d*f)
    evaluator.add(ae_plus_bf, ce_plus_df, ae_plus_bf_plus_ce_plus_df);
    std::cout << "finished (a*e + b*f) + (c*e + d*f)"<< std::endl;//1958182 1

    Ciphertext trivial_result;
    evaluator.sub(e_times_a_plus_c_plus_f_times_b_plus_d, ae_plus_bf_plus_ce_plus_df, trivial_result);



    Plaintext decrypted_trivial_result;
    decryptor.decrypt(trivial_result, decrypted_trivial_result);
    assert(decrypted_trivial_result.is_zero());

    Plaintext result_way_one;
    decryptor.decrypt(e_times_a_plus_c_plus_f_times_b_plus_d, result_way_one);

    Plaintext result_way_two;
    decryptor.decrypt(ae_plus_bf_plus_ce_plus_df, result_way_two);
    assert(result_way_one == result_way_two);

    assert(trivial_result.is_transparent());

    std::cout << "Worked" << std::endl;

    return 0;
}

