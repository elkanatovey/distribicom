#include "test_utils.hpp"

namespace TestUtils {
    void set_enc_params(uint32_t N, uint32_t logt, seal::EncryptionParameters &enc_params) {
        std::cout << "Main: Generating SEAL parameters" << std::endl;
        enc_params.set_poly_modulus_degree(N);
        enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
        enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));
    }

    std::shared_ptr<CryptoObjects> setup(TestUtils::SetupConfigs configs) {
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


    CryptoObjects::CryptoObjects(seal::EncryptionParameters encryption_params) :
            encryption_params(encryption_params),
            seal_context(encryption_params, true),
            keygen(seal::SEALContext(seal_context)),
            secret_key(keygen.secret_key()),
            encryptor(seal::Encryptor(seal::SEALContext(seal_context), secret_key)),
            decryptor(seal::Decryptor(seal::SEALContext(seal_context), secret_key)),
            encoder(seal::BatchEncoder(seal::SEALContext(seal_context))) {
        //stoeln from seal pir. the needed rotation keys are generated here.
        std::vector<uint32_t> galois_elts;
        int N = encryption_params.poly_modulus_degree();
        int logN = seal::util::get_power_of_two(N);

        // cout << "printing galois elements...";
        for (int i = 0; i < logN; i++) {
            galois_elts.push_back((N + seal::util::exponentiate_uint(2, i)) /
                                  seal::util::exponentiate_uint(2, i));
        }
        keygen.create_galois_keys(galois_elts, gal_keys);

        w_evaluator = math_utils::EvaluatorWrapper::Create(encryption_params);
    }

    seal::GaloisKeys CryptoObjects::generate_galois_keys() {
        // Generate the Galois keys needed for coeff_select.
        std::vector<uint32_t> galois_elts;
        std::uint64_t N = encryption_params.poly_modulus_degree();
        std::uint64_t logN = seal::util::get_power_of_two(N);

        // cout << "printing galois elements...";
        for (std::uint64_t i = 0; i < logN; i++) {
            galois_elts.push_back((N + seal::util::exponentiate_uint(2, i)) /
                                  seal::util::exponentiate_uint(2, i));
            //#ifdef DEBUG
            // cout << galois_elts.back() << ", ";
            //#endif
        }
        seal::GaloisKeys gal_keys;
        keygen.create_galois_keys(galois_elts, gal_keys);
        return gal_keys;
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

    long time_func(const std::function<void()> &f) {
        auto start = std::chrono::high_resolution_clock::now();
        f();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        return duration.count();
    }

    void time_func_print(const std::string &name, const std::function<void()> &f) {
        std::cout << name << ": " << time_func(f) / 1000000 << "ms" << std::endl;
    }
}