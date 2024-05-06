#include <iostream>
#include <filesystem>
#include <seal/seal.h>
#include "marshal/local_storage.hpp"
#include "services/factory.hpp"
#include "utils.hpp"
#include "evaluator_wrapper.hpp"


seal::Plaintext random_ptx(seal::EncryptionParameters &encryption_params) {

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

seal::Ciphertext random_ctx(seal::EncryptionParameters &enc_params) {
    seal::SEALContext seal_context(enc_params, true);
    seal::KeyGenerator key_gen(seal_context);
    seal::SecretKey secret_key(key_gen.secret_key());
    seal::Encryptor encryptor(seal::Encryptor(seal::SEALContext(seal_context), secret_key));

    auto ptx = random_ptx(enc_params);
    seal::Ciphertext ctx;
    encryptor.encrypt_symmetric(ptx, ctx);

    return ctx;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <num_repetions>" << std::endl;
        return 1;
    }

    int num_repetions = std::stoi(argv[1]);

    auto c = services::configurations::create_app_configs(
            "",
            4096,//N
            20, //ptx coefs bit size
            42, // dpir rownum doesn't matter for this benchmark
            41, // dpir colnum doesn't matter for this benchmark
            256, // element per ptxs
            1, // dpir numworker doesn't matter for this benchmark
            10, // irelevant.
            1 // num client doesn't matter for this benchmark
    );
    seal::EncryptionParameters enc_params = services::utils::setup_enc_params(c);

    std::cout << "starting benchmark:" << std::endl;

    auto w_evaluator = math_utils::EvaluatorWrapper::Create(enc_params);
    auto ctx = random_ctx(enc_params);
    auto ctx2 = random_ctx(enc_params);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_repetions; ++i) {
        w_evaluator->old_scalar_multiply(i, ctx, ctx2);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "old_scalar_mult total time: " << double(duration.count()) / double(num_repetions) << " ms"
              << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_repetions; ++i) {
        w_evaluator->scalar_multiply(i, ctx, ctx2);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "scalar_mult total time: " << double(duration.count()) / double(num_repetions) << " ms" << std::endl;

    return 0;
}
