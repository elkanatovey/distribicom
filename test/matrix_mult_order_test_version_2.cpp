//
// test that  ([x, y] * [[a, b], [c, d]]) * [e, f]^T = [x, y] * ([[a, b], [c, d]] * [e, f]^T) in the ciphertext space
// were e, f are both ciphertexts and x, y are both set to 1.
//
// in other words, test that e * (a + c) + f * (b + d) = (a*e + b*f) + (c*e + d*f)
//


#include <cassert>
#include <cstdint>
#include <memory>
#include <seal/seal.h>



using namespace std::chrono;
using namespace std;
using namespace seal;

void add_plaintexts(const EncryptionParameters &enc_params, const Plaintext &a, const Plaintext &c, Plaintext &a_plus_c);

int main(int argc, char *argv[]) {

    uint32_t N = 4096;

    uint32_t logt = 20;

    EncryptionParameters enc_params(scheme_type::bgv);

    // Generates all parameters

    cout << "Main: Generating SEAL parameters" << endl;
    enc_params.set_poly_modulus_degree(N);
    enc_params.set_coeff_modulus(CoeffModulus::BFVDefault(N));
    enc_params.set_plain_modulus(PlainModulus::Batching(N, logt + 1));

    SEALContext context(enc_params, true);
    KeyGenerator keygen(context);

    SecretKey secret_key = keygen.secret_key();
    Encryptor encryptor(context, secret_key);
    Decryptor decryptor(context, secret_key);
    Evaluator evaluator(context);


    cout << "Main: SEAL parameters generated" << endl;

    cout << "Main: generating matrices" << endl;


    std::string poly = "123D7D"; // multiples with/without mod operator: "247AFA" "51AF9"
    Plaintext a(poly);         // example poly that doesn't work at all:" 1CBB5Bx^3 + A59B8x^2 + 6C3D2x^1 + 123D7D"
    Plaintext b(poly);
    Plaintext c(poly);
    Plaintext d(poly);
    Plaintext e(poly);
    Plaintext f(poly);


    Ciphertext e_encrypted;
    Ciphertext f_encrypted;

    encryptor.encrypt_symmetric(e, e_encrypted);
    encryptor.encrypt_symmetric(f, f_encrypted);
    std::cout << "Encrypting e f" << std::endl;
    std::cout << "Plain Modulus:"<<enc_params.plain_modulus().value() << std::endl;

    std::cout << "calculating e * (a + c) + f * (b + d)"<< std::endl;

    Plaintext a_plus_c;
    Plaintext b_plus_d;

    add_plaintexts(enc_params, a, c, a_plus_c);
    add_plaintexts(enc_params, b, d, b_plus_d);

    std::cout << a_plus_c.to_string()<<" plaintext addition result..........................."<< std::endl;
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

void add_plaintexts(const EncryptionParameters &enc_params, const Plaintext &a, const Plaintext &c, Plaintext &a_plus_c) {
    Plaintext to_add;
    if(a.coeff_count() > c.coeff_count()){
        a_plus_c = a;
        to_add = c;
    }
    else{
        a_plus_c = c;
        to_add = a;
    }
    for(int current_coeff =0; current_coeff < to_add.coeff_count(); current_coeff++){
        a_plus_c[current_coeff] = (a_plus_c[current_coeff] + to_add[current_coeff])%enc_params.plain_modulus().value();
    }
}

