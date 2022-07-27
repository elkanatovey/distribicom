#include <seal/seal.h>
#include "matrix_multiplier_base.hpp"

using namespace std;
using namespace seal;


void set_enc_params(uint32_t N, uint32_t logt, EncryptionParameters &enc_params) {
    cout << "Main: Generating SEAL parameters" << endl;
    enc_params.set_poly_modulus_degree(N);
    enc_params.set_coeff_modulus(CoeffModulus::BFVDefault(N));
    enc_params.set_plain_modulus(PlainModulus::Batching(N, logt + 1));
}

int main(int argc, char *argv[]) {
    // sanity check
    multiplication_utils::foo();

    uint32_t N = 4096;

    uint32_t logt = 20;
    auto rows = 20;
    auto cols = 4;
    EncryptionParameters enc_params(scheme_type::bgv);

    function<uint32_t(void)> random_val_generator;
    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();
    random_val_generator =[&gen](){return gen->generate() % 2; };


    // Generates all parameters

    set_enc_params(N, logt, enc_params);

    SEALContext context(enc_params, true);
    KeyGenerator keygen(context);
    auto evaluator_ = make_shared<Evaluator>(context);

    auto s=Ciphertext();
    SecretKey secret_key = keygen.secret_key();
    Encryptor encryptor(context, secret_key);
    Decryptor decryptor(context, secret_key);
    Evaluator evaluator(context);


    cout << "Main: SEAL parameters generated" << endl;

    cout << "Main: generating matrices" << endl;

    std::vector<Plaintext> matrix(rows*cols);
    BatchEncoder encoder(context);
    std::vector<std::uint64_t> random_values(N, 0ULL);
    for(auto& elem: matrix){
        generate(random_values.begin(), random_values.end(), random_val_generator);
        encoder.encode(random_values, elem);
    }


    multiplication_utils::matrix_multiplier::Create(evaluator_, enc_params);


}

