//
// Created by Jonathan Weiss on 28/07/2022.
//

#include "matrix_multiplier_base.hpp"
#include <seal/seal.h>

namespace TestUtils {
    void set_enc_params(uint32_t N, uint32_t logt, seal::EncryptionParameters &enc_params) {
        cout << "Main: Generating SEAL parameters" << endl;
        enc_params.set_poly_modulus_degree(N);
        enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
        enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));
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

    struct CryptoObjects {

        explicit CryptoObjects(seal::EncryptionParameters encryption_params) :
                encryption_params(encryption_params),
                seal_context(encryption_params, true),
                keygen(seal::SEALContext(seal_context)),
                secret_key(keygen.secret_key()),
                evaluator(seal::SEALContext(seal_context)),
                encryptor(seal::Encryptor(seal::SEALContext(seal_context), secret_key)),
                decryptor(seal::Decryptor(seal::SEALContext(seal_context), secret_key)),
                encoder(seal::BatchEncoder(seal::SEALContext(seal_context))) {
            shared_evaluator = std::make_shared<seal::Evaluator>(seal::SEALContext(seal_context));
        };

        seal::EncryptionParameters encryption_params;
        function<uint32_t(void)> rng;
        seal::SEALContext seal_context;
        seal::KeyGenerator keygen;
        seal::SecretKey secret_key;

        std::shared_ptr<seal::Evaluator> shared_evaluator;
        seal::Evaluator evaluator;

        seal::Encryptor encryptor;
        seal::Decryptor decryptor;
        seal::BatchEncoder encoder;
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

    std::shared_ptr<CryptoObjects> setup(SetupConfigs configs) {
        seal::EncryptionParameters enc_params(configs.encryption_params_configs.scheme_type);
        set_enc_params(
                configs.encryption_params_configs.polynomial_degree,
                configs.encryption_params_configs.log_coefficient_modulus,
                enc_params
        );

        auto s = std::make_shared<CryptoObjects>(enc_params);
        s->rng = setup_rng(configs.rng_configs);
        return s;
    }

    SetupConfigs DEFAULT_SETUP_CONFIGS = {
            .encryption_params_configs = {
                    .scheme_type = seal::scheme_type::bgv,
                    .polynomial_degree = 4096,
                    .log_coefficient_modulus = 20
            },
            .rng_configs = {
                    .rng_type = blake,
            }

    };
}