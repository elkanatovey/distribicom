#pragma once

#include "matrix_multiplier.hpp"
#include <seal/seal.h>
#include <evaluator_wrapper.hpp>
#include <pir.hpp>

namespace TestUtils {
    void set_enc_params(uint32_t N, uint32_t logt, seal::EncryptionParameters &enc_params);

    struct TestEncryptionParamsConfigs {
        seal::scheme_type scheme_type;
        std::uint32_t polynomial_degree; // N
        std::uint32_t log_coefficient_modulus; // log(T)
    };
    enum RNGType {
        blake, sha, all_ones
    };

    struct TestPIRParamsConfigs {
        std::uint64_t number_of_items;
        std::uint64_t size_per_item;
        std::uint64_t dimensions;
        bool use_symmetric, use_batching, use_recursive_mod_switching;
    };

    struct SetupConfigs {
        TestEncryptionParamsConfigs encryption_params_configs;
        TestPIRParamsConfigs pir_params_configs;
    };

    struct CryptoObjects {

        explicit CryptoObjects(seal::EncryptionParameters encryption_params);

        seal::EncryptionParameters encryption_params;
        PirParams pir_params;

        std::function<uint32_t(void)> blakerng;
        std::function<uint32_t(void)> mod2rng;
        std::function<uint32_t(void)> allonerng;

        seal::SEALContext seal_context;
        seal::KeyGenerator keygen;
        seal::SecretKey secret_key;
        std::shared_ptr<multiplication_utils::EvaluatorWrapper> w_evaluator;


        seal::Encryptor encryptor;
        seal::Decryptor decryptor;
        seal::BatchEncoder encoder;

        seal::Plaintext random_plaintext();

        seal::Ciphertext random_ciphertext();

        seal::GaloisKeys generate_galois_keys();
    };

    std::shared_ptr<CryptoObjects> setup(SetupConfigs configs);

    extern TestEncryptionParamsConfigs DEFAULT_ENCRYPTION_PARAMS_CONFIGS;
    extern TestPIRParamsConfigs DEFAULT_PIR_PARAMS_CONFIGS;

    extern SetupConfigs DEFAULT_SETUP_CONFIGS;

    long time_func(const std::function<void()> &f);

    void time_func_print(const std::string &name, const std::function<void()> &f);
}