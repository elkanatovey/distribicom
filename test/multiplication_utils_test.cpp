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

struct TestEncryptionParamsConfigs {
    seal::scheme_type scheme_type;
    std::uint32_t polynomial_degree; // N
    std::uint32_t log_coefficient_modulus; // log(T)
};
enum RNGType {
    blake, sha, all_ones
};

struct RNGConfigs {
    std::uint64_t seed;
    RNGType rng_type;
};
struct SetupConfigs {
    TestEncryptionParamsConfigs encryption_params_configs;
    RNGConfigs rng_configs;

};

//TODO rename
struct SetupOutput {
    seal::EncryptionParameters encryption_params;
    function<uint32_t(void)> rng;
};

function<uint32_t(void)> setup_rng(RNGConfigs configs) {
    function<uint32_t(void)> random_val_generator;
    if (configs.rng_type == blake) {
        seal::Blake2xbPRNGFactory factory;
        auto gen = factory.create();
        return [gen = std::move(gen)]() { return gen->generate() % 2; };
    }
    else if (configs.rng_type == sha) {
        throw std::runtime_error("not implemented");
    }
    else if (configs.rng_type == all_ones) {
        return []() { return 1; };
    }
    throw std::runtime_error("unknown rng type");
}

SetupOutput setup(SetupConfigs configs) {
    seal::EncryptionParameters enc_params(configs.encryption_params_configs.scheme_type);
    set_enc_params(
            configs.encryption_params_configs.polynomial_degree,
            configs.encryption_params_configs.log_coefficient_modulus,
            enc_params
    );
    auto rng = setup_rng(configs.rng_configs);

    return SetupOutput{
            .encryption_params = enc_params,
            .rng = rng
    };
}

int main(int argc, char *argv[]) {
    // sanity check
    multiplication_utils::foo();

    auto all = setup(SetupConfigs{
            .encryption_params_configs =  {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096,
                    .log_coefficient_modulus = 20
            },
    });

    auto rows = 20;
    auto cols = 4;

    auto random_val_generator = all.rng;


    // Generates all parameters

    SEALContext context(all.encryption_params, true);
    KeyGenerator keygen(context);
    auto evaluator_ = make_shared<Evaluator>(context);

    auto s = Ciphertext();
    SecretKey secret_key = keygen.secret_key();
    Encryptor encryptor(context, secret_key);
    Decryptor decryptor(context, secret_key);
    Evaluator evaluator(context);


    cout << "Main: SEAL parameters generated" << endl;

    cout << "Main: generating matrices" << endl;

    std::vector<Plaintext> matrix(rows * cols);
    BatchEncoder encoder(context);
    std::vector<std::uint64_t> random_values(all.encryption_params.poly_modulus_degree(), 0ULL);
    for (auto &elem: matrix) {
        generate(random_values.begin(), random_values.end(), random_val_generator);
        encoder.encode(random_values, elem);
    }


    auto matrix_multiplier = multiplication_utils::matrix_multiplier::Create(evaluator_, all.encryption_params);


}

