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


public:
    FreivaldsVector(const seal::EncryptionParameters &enc_params, const PirParams& params);

    /**
     * vector always multiplies from left
     */
    void multiply_with_db( std::vector<std::vector<std::uint64_t>> &db_unencoded);

    static void multiply_add(std::vector<uint64_t> &left, std::vector<uint64_t> &right, std::vector<std::uint64_t>
            &result);
};


#endif //DISTRIBICOM_FREIVALDSVECTOR_HPP
