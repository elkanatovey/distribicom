#include "matrix_multiplier_base.hpp"
#include <seal/seal.h>
#include <evaluator_wrapper.hpp>

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

        explicit CryptoObjects(seal::EncryptionParameters encryption_params) :
                encryption_params(encryption_params),
                seal_context(encryption_params, true),
                keygen(seal::SEALContext(seal_context)),
                secret_key(keygen.secret_key()),
                encryptor(seal::Encryptor(seal::SEALContext(seal_context), secret_key)),
                decryptor(seal::Decryptor(seal::SEALContext(seal_context), secret_key)),
                encoder(seal::BatchEncoder(seal::SEALContext(seal_context))) {

            w_evaluator = multiplication_utils::EvaluatorWrapper::Create(encryption_params);
        };

        seal::EncryptionParameters encryption_params;
        PirParams pir_params;

        function<uint32_t(void)> blakerng;
        function<uint32_t(void)> mod2rng;
        function<uint32_t(void)> allonerng;

        seal::SEALContext seal_context;
        seal::KeyGenerator keygen;
        seal::SecretKey secret_key;
        std::shared_ptr<multiplication_utils::EvaluatorWrapper> w_evaluator;


        seal::Encryptor encryptor;
        seal::Decryptor decryptor;
        seal::BatchEncoder encoder;

        seal::Plaintext random_plaintext();

        seal::Ciphertext random_ciphertext();
    };

    std::shared_ptr<CryptoObjects> setup(SetupConfigs configs) {
        seal::EncryptionParameters enc_params(configs.encryption_params_configs.scheme_type);
        set_enc_params(
                configs.encryption_params_configs.polynomial_degree,
                configs.encryption_params_configs.log_coefficient_modulus,
                enc_params
        );

        verify_encryption_params(enc_params);

        auto s = std::make_shared<CryptoObjects>(enc_params);
        // initializing pir_parameters.
        auto pir_configs = configs.pir_params_configs;
        gen_pir_params(pir_configs.number_of_items,
                       pir_configs.size_per_item,
                       pir_configs.dimensions,
                       s->encryption_params,
                       s->pir_params,
                       pir_configs.use_symmetric,
                       pir_configs.use_batching,
                       pir_configs.use_recursive_mod_switching
        );
        return s;
    }

    TestEncryptionParamsConfigs DEFAULT_ENCRYPTION_PARAMS_CONFIGS = {
            .scheme_type = seal::scheme_type::bgv,
            .polynomial_degree = 4096,
            .log_coefficient_modulus = 20
    };

    TestPIRParamsConfigs DEFAULT_PIR_PARAMS_CONFIGS = {
            .number_of_items = 100,
            .size_per_item = 100,
            .dimensions = 1,
            .use_symmetric = true,
            .use_batching = true,
            .use_recursive_mod_switching = true
    };

    SetupConfigs DEFAULT_SETUP_CONFIGS = {
            .encryption_params_configs = DEFAULT_ENCRYPTION_PARAMS_CONFIGS,
            .pir_params_configs = DEFAULT_PIR_PARAMS_CONFIGS,
    };

    seal::Plaintext CryptoObjects::random_plaintext() {
        seal::Plaintext p(encryption_params.poly_modulus_degree());

        // avoiding encoder usage - to prevent unwanted transformation to the ptx underlying elements
        std::vector<std::uint64_t> v(encryption_params.poly_modulus_degree(), 0);

        seal::Blake2xbPRNGFactory factory;
        auto gen = factory.create();

        auto mod = encryption_params.plain_modulus().value();
        std::generate(v.begin(), v.end(), [gen = std::move(gen), &mod]() { return gen->generate() % mod; });

        for (std::uint64_t i = 0; i < encryption_params.poly_modulus_degree(); ++i) {
            p[i] = v[i];
        }
        return p;
    }

    seal::Ciphertext CryptoObjects::random_ciphertext() {
        auto ptx = random_plaintext();
        seal::Ciphertext ctx;
        encryptor.encrypt_symmetric(ptx, ctx);
        return ctx;
    }
}