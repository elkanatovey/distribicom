#include "../test_utils.hpp"
#include "matrix_operations.hpp"


// 1753ms 1 core
// 646ms

//2713ms 1 core
// 980ms 6 core

int mempool_test(int a, char *b[]) {
    auto max_cores = 8;
    auto n_thread = 8;
    concurrency::threadpool pool(n_thread);

    auto num_additions_per_thread = (50000 * max_cores) / n_thread;
    std::cout << "num_additions_per_thread: " << num_additions_per_thread << " per thread" << std::endl;
    concurrency::safelatch latch((int) (n_thread));
    seal::MemoryManager::SwitchProfile(std::make_unique<seal::MMProfThreadLocal>());

    auto time = TestUtils::time_func([&]() {
        for (int i = 0; i < n_thread; ++i) {
            pool.submit(
                    {
                            .f =[&,num_additions_per_thread, i] {
                                uint32_t N = 8192;
                                uint32_t logt = 20;
                                seal::EncryptionParameters enc_params(seal::scheme_type::bgv);
                                gen_encryption_params(N, logt, enc_params);
                                verify_encryption_params(enc_params);
                                seal::SEALContext context(enc_params, true);
                                seal::KeyGenerator keygen(context);
                                seal::SecretKey secret_key = keygen.secret_key();
                                seal::Encryptor encryptor(context, secret_key);
                                seal::Evaluator evaluator(context);
                                seal::BatchEncoder encoder(context);


                                seal::MemoryPoolHandle mempool = seal::MemoryPoolHandle::ThreadLocal();

                                seal::Plaintext p_a(enc_params.poly_modulus_degree(), mempool);
                                seal::Plaintext p_b(enc_params.poly_modulus_degree(), mempool);

                                std::vector<std::uint64_t> v(enc_params.poly_modulus_degree(), 0);

                                seal::Blake2xbPRNGFactory factory;
                                auto gen = factory.create();
                                std::generate(v.begin(), v.end(), [gen = std::move(gen)]() { return gen->generate() % 256; });
                                for (std::uint64_t i = 0; i < enc_params.poly_modulus_degree(); ++i) {
                                    p_a[i] = v[i];
                                    p_b[i] = v[i] / 2;
                                }

                                seal::Ciphertext a(context, mempool);
                                seal::Ciphertext b(context, mempool);
                                encryptor.encrypt_symmetric(p_a, a, mempool);
                                encryptor.encrypt_symmetric(p_b, b, mempool);

                                for (int i = 0; i < num_additions_per_thread; ++i) {

                                    evaluator.add_inplace(a, b);
                                }
                                latch.count_down();
                            }
                    }
            );
        }
        latch.wait();
    });

    std::cout << "total-time: " << time << "ms" << std::endl;
    return 0;
}