#include "FreivaldsVector.hpp"

#define COL 0
#define ROW 1
namespace {
    void multiply_add(std::uint64_t left, seal::Plaintext &right,
                      seal::Plaintext &result, uint64_t mod);
}
FreivaldsVector::FreivaldsVector(const seal::EncryptionParameters &enc_params, const PirParams& pir_params,
                                 std::function<uint32_t ()>& gen)
:enc_params_(enc_params),
pir_params_(pir_params){  //generates the freeivalds vec

    context_ = std::make_shared<seal::SEALContext>(enc_params, true);
    evaluator_ = std::make_unique<seal::Evaluator>(*context_);
    encoder_ = std::make_unique<seal::BatchEncoder>(*context_);

    auto num_of_rows = pir_params_.nvec[ROW];

    std::vector<std::uint64_t> freivalds_vector(num_of_rows);
    generate(freivalds_vector.begin(), freivalds_vector.end(), gen);

    this->random_vec = std::move(freivalds_vector);
}


/**
 * do (random_vector*db) and encode result as plaintext
 * @param db_unencoded
 */
template<>
void FreivaldsVector::mult_rand_vec_by_db(const std::shared_ptr<Database>& db) {
    auto num_of_rows = pir_params_.nvec[ROW];
    auto num_of_cols = pir_params_.nvec[COL];

    std::vector<seal::Plaintext> result_vec;
    result_vec.reserve(num_of_cols);

    // reserve space for multiplication result
    for(uint64_t l = 0; l < num_of_cols; l++){
        seal::Plaintext partial_result(enc_params_.poly_modulus_degree());
        result_vec.push_back(partial_result);
    }

    //do multiplication. k row index j col index
    for(uint64_t k = 0; k < num_of_cols; k++) {
        for (uint64_t j = 0; j < num_of_rows; j++) {
            multiply_add(random_vec[j], (*(db))[j + k * num_of_rows], result_vec[k], enc_params_.plain_modulus().value());
        }
    }

    for(uint64_t i = 0; i < num_of_cols; i++){
        evaluator_->transform_to_ntt_inplace(result_vec[i], context_->first_parms_id());
    }

    random_vec_mul_db = std::move(result_vec);
}



/**
 * execute (freivalds_vec*db)*query with an expanded query
 * @param query_id
 * @param query
 */
void FreivaldsVector::multiply_with_query(uint32_t query_id, const std::vector<seal::Ciphertext>& query){

    std::vector<seal::Ciphertext> temp_storage;
    temp_storage.reserve(random_vec_mul_db.size());

    for(int i=0; i<random_vec_mul_db.size(); i++){
        seal::Ciphertext temp;
        evaluator_->multiply_plain(query[i],random_vec_mul_db[i] , temp);
        temp_storage.push_back(std::move(temp));
    }
    seal::Ciphertext result;
    evaluator_->add_many(temp_storage, result);
    random_vec_mul_db_mul_query[query_id] = std::move(result);
}

/**
 * execute freivalds_vec*(db*query) for single db shard. This essentially checks single ciphertext
 * @param query_id
 * @param db_shard_id
 * @param reply
 * @return
 */
bool FreivaldsVector::multiply_with_reply(uint32_t query_id, PirReply &reply, seal::Ciphertext& response) {
    std::vector<seal::Ciphertext> temp_storage;
    temp_storage.reserve(reply.size());


    seal::Ciphertext result(*context_, seal::MemoryManager::GetPool());
    evaluator_->transform_to_ntt_inplace(result);
    for(int i=0; i < random_vec.size(); i++){
        seal::Ciphertext temp(*context_, seal::MemoryManager::GetPool());
        evaluator_->transform_to_ntt_inplace(temp);
        if(random_vec[i]==1){
            temp = reply[i];
        }
        temp_storage.push_back(std::move(temp));
    }
    evaluator_->add_many(temp_storage, result);
//    seal::Ciphertext difference;
    std::cout<< "checking if result is transparent"<<std::endl;
    evaluator_->sub(random_vec_mul_db_mul_query[query_id], result, response);
    evaluator_->transform_from_ntt_inplace(response);
    if(response.is_transparent()){
        std::cout<< "transparent!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
        return true;
    }
    std::cout<< "result is not transparent"<<std::endl;
    return false;
}

namespace {
    /**
     * take vector along with constant and do result+= vector*constant
     * @param right vector from above
     * @param left const
     * @param result place to add to
     */
    void multiply_add(std::uint64_t left, seal::Plaintext &right,
                      seal::Plaintext &result, uint64_t mod) {
        if(left==0){
            return;
        }
        auto coeff_count = right.coeff_count();
        for(int current_coeff =0; current_coeff < coeff_count; current_coeff++){
            result[current_coeff] +=(left * right[current_coeff]);
            if(result[current_coeff]>=mod){
                result[current_coeff]-=mod;
            }
        }

    }
}
