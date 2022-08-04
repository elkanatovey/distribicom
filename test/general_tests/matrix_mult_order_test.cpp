//
// test that  ([x, y] * [[a, b], [c, d]]) * [e, f]^T = [x, y] * ([[a, b], [c, d]] * [e, f]^T) in the ciphertext space
// were e, f are both ciphertexts and x, y are both set to 1.
//
// in other words, test that e * (a + c) + f * (b + d) = (a*e + b*f) + (c*e + d*f)
//


#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <seal/seal.h>
#include "../test_utils.hpp"


using namespace std::chrono;
using namespace std;
using namespace seal;


int matrix_mult_order_test(int argc, char *argv[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    size_t slot_count = all->encoder.slot_count();


    cout << "Main: SEAL parameters generated" << endl;

    cout << "Main: generating matrices" << endl;
    vector<uint64_t> a_arr(slot_count, 0ULL);
    vector<uint64_t> b_arr(slot_count, 0ULL);
    vector<uint64_t> c_arr(slot_count, 0ULL);
    vector<uint64_t> d_arr(slot_count, 0ULL);
    vector<uint64_t> e_arr(slot_count, 0ULL);
    vector<uint64_t> f_arr(slot_count, 0ULL);

    for (uint32_t i = 0; i < a_arr.size(); i++) {
        a_arr[i] = 514048;
        b_arr[i] = (rand() % 256);
        c_arr[i] = 514050;
        d_arr[i] = (rand() % 256);
        e_arr[i] = (rand() % 256);
        f_arr[i] = (rand() % 256);
    }

    std::string poly = "123D7D";
    std::string polydub = "247AFA";
//    Plaintext a(poly);
//    Plaintext b(poly);
//    Plaintext c(poly);
//    Plaintext d(poly);
    Plaintext e(poly);
    Plaintext f(poly);
    Plaintext a(4096);
    Plaintext b(4096);
    Plaintext c(4096);
    Plaintext d(4096);
    for (int current_coeff = 0; current_coeff < 4096; current_coeff++) {
        a[current_coeff] = a_arr[current_coeff];
        b[current_coeff] = b_arr[current_coeff];
        c[current_coeff] = c_arr[current_coeff];
        d[current_coeff] = d_arr[current_coeff];
    }

//    Plaintext e;
//    Plaintext f;
//        encoder.encode(a_arr, a);
//        encoder.encode(b_arr, b);
//        encoder.encode(c_arr, c);
//        encoder.encode(d_arr, d);
    all->encoder.encode(e_arr, e);
    all->encoder.encode(f_arr, f);

    Ciphertext e_encrypted;
    Ciphertext f_encrypted;

    all->encryptor.encrypt_symmetric(e, e_encrypted);
    all->encryptor.encrypt_symmetric(f, f_encrypted);
    std::cout << "Encrypting e f" << std::endl;
    std::cout << "Plain Modulus:" << all->encryption_params.plain_modulus().value() << std::endl;

    std::cout << "calculating e * (a + c) + f * (b + d)" << std::endl;

    Plaintext a_plus_c;
    Plaintext b_plus_d;

    all->w_evaluator->add_plain(a, c, a_plus_c);
    all->w_evaluator->add_plain(b, d, b_plus_d);

    std::cout << a_plus_c.to_string() << " ..........................." << std::endl;
    Ciphertext e_times_a_plus_c; // e * (a + c)
    Ciphertext f_times_b_plus_d; // f * (b + d)
    all->w_evaluator->evaluator->multiply_plain(e_encrypted, a_plus_c, e_times_a_plus_c);
    all->w_evaluator->evaluator->multiply_plain(f_encrypted, b_plus_d, f_times_b_plus_d);

    Ciphertext e_times_a_plus_c_plus_f_times_b_plus_d; // e * (a + c) + f * (b + d)
    all->w_evaluator->evaluator->add(e_times_a_plus_c, f_times_b_plus_d, e_times_a_plus_c_plus_f_times_b_plus_d);

    std::cout << "finished e * (a + c) + f * (b + d)" << std::endl;//1958182 1


    std::cout << "calculating (a*e + b*f) + (c*e + d*f)" << std::endl;//1958182 1
    Ciphertext a_times_e; // a * e
    Ciphertext b_times_f; // b * f
    Ciphertext c_times_e; // c * e
    Ciphertext d_times_f; // d * f
    all->w_evaluator->evaluator->multiply_plain(e_encrypted, a, a_times_e);
    all->w_evaluator->evaluator->multiply_plain(f_encrypted, b, b_times_f);
    all->w_evaluator->evaluator->multiply_plain(e_encrypted, c, c_times_e);
    all->w_evaluator->evaluator->multiply_plain(f_encrypted, d, d_times_f);

    Ciphertext ae_plus_bf; // (a*e + b*f)
    Ciphertext ce_plus_df; // (c*e + d*f)
    all->w_evaluator->evaluator->add(a_times_e, b_times_f, ae_plus_bf);
    all->w_evaluator->evaluator->add(c_times_e, d_times_f, ce_plus_df);

    Ciphertext ae_plus_bf_plus_ce_plus_df; // (a*e + b*f) + (c*e + d*f)
    all->w_evaluator->evaluator->add(ae_plus_bf, ce_plus_df, ae_plus_bf_plus_ce_plus_df);
    std::cout << "finished (a*e + b*f) + (c*e + d*f)" << std::endl;//1958182 1

    Ciphertext trivial_result;
    all->w_evaluator->evaluator->sub(e_times_a_plus_c_plus_f_times_b_plus_d, ae_plus_bf_plus_ce_plus_df,
                                     trivial_result);


    Plaintext decrypted_trivial_result;
    all->decryptor.decrypt(trivial_result, decrypted_trivial_result);
    assert(decrypted_trivial_result.is_zero());

    Plaintext result_way_one;
    all->decryptor.decrypt(e_times_a_plus_c_plus_f_times_b_plus_d, result_way_one);

    Plaintext result_way_two;
    all->decryptor.decrypt(ae_plus_bf_plus_ce_plus_df, result_way_two);
    assert(result_way_one == result_way_two);

    assert(trivial_result.is_transparent()); // TODO: this assert fails

    std::cout << "Worked" << std::endl;

    return 0;
}

