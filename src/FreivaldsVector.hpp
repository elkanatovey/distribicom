#ifndef DISTRIBICOM_FREIVALDSVECTOR_HPP
#define DISTRIBICOM_FREIVALDSVECTOR_HPP

#include "distributed_pir.hpp"

class FreivaldsVector {
    PirParams pir_params_;       // PIR parameters
    seal::EncryptionParameters enc_params_; // SEAL parameters
    std::unique_ptr<seal::Evaluator> evaluator_;
    std::unique_ptr<seal::BatchEncoder> encoder_;
    std::shared_ptr<seal::SEALContext> context_;

    std::vector<std::vector<std::uint64_t>> random_vec;
    std::vector<seal::Plaintext> random_vec_mul_db;

    std::map<uint32_t, seal::Plaintext> random_vec_mul_db_mul_query;


public:
    FreivaldsVector(const seal::EncryptionParameters &enc_params, const PirParams& params);

    /**
     * vector always multiplies from left
     */
    void multiply_with_db( std::vector<std::vector<std::uint64_t>> &db_unencoded);

    /**
     * multiply (A*B)*C where (A*B) where A is freivalds vector, B is DB and C is an individual query. results are
     * stored for future comparison
     */
    void multiply_with_query(uint32_t client_id, const std::vector<seal::Ciphertext> & query);

private:
    static void multiply_add(std::vector<uint64_t> &left, std::vector<uint64_t> &right, std::vector<std::uint64_t>
            &result);
};


#endif //DISTRIBICOM_FREIVALDSVECTOR_HPP
