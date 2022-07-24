#ifndef DISTRIBICOM_FREIVALDSVECTOR_HPP
#define DISTRIBICOM_FREIVALDSVECTOR_HPP

#include "distributed_pir.hpp"

struct RandomVecMulDbContainer {
    std::vector<seal::Plaintext> plain_version {};
    std::vector<seal::Ciphertext> enc_version {};
};

class FreivaldsVector {
    PirParams pir_params_;       // PIR parameters
    seal::EncryptionParameters enc_params_; // SEAL parameters
    std::unique_ptr<seal::Evaluator> evaluator_;
    std::shared_ptr<seal::SEALContext> context_;

    std::vector<std::uint64_t> random_vec;
    RandomVecMulDbContainer random_vec_mul_db;

    std::map<uint32_t, seal::Ciphertext> random_vec_mul_db_mul_query;


public:
    FreivaldsVector(const seal::EncryptionParameters &enc_params, const PirParams& params, std::function<uint32_t ()>&
            gen);

    /**
     * vector always multiplies from left
     */
    template<class T>
    void mult_rand_vec_by_db(const std::shared_ptr<T>& db);

    /**
     * multiply (A*B)*C where (A*B) where A is freivalds vector, B is DB and C is an individual query. results are
     * stored for future comparison
     */
    void multiply_with_query(uint32_t query_id, const std::vector<seal::Ciphertext> & query);


    /**
     * multiply A*(B*C)  where A is freivalds vector, B is DB and C is an individual query. (B*C) is the
     * partial reply. Results are compared with random_vec_mul_db_mul_query[client_id]
     */
    bool multiply_with_reply(uint32_t query_id, PirReply &reply, seal::Ciphertext& response);
};


#endif //DISTRIBICOM_FREIVALDSVECTOR_HPP
