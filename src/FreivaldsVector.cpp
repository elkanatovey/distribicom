#include "FreivaldsVector.hpp"

#define COL 0
#define ROW 1

FreivaldsVector::FreivaldsVector(const seal::EncryptionParameters &enc_params, const PirParams& pir_params):enc_params_(enc_params),
pir_params_(pir_params){  //generates the freeivalds vec

    context_ = std::make_shared<seal::SEALContext>(enc_params, true);
    evaluator_ = std::make_unique<seal::Evaluator>(*context_);
    encoder_ = std::make_unique<seal::BatchEncoder>(*context_);

    auto num_of_rows = pir_params_.nvec[ROW];

//    std::random_device rd; // @todo this needs to be implemented with a cprng
    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();

    std::vector<std::uint64_t> freivalds_vector(num_of_rows);
    generate(freivalds_vector.begin(), freivalds_vector.end(), [&gen](){return gen->generate()%2; });

    this->random_vec = std::move(freivalds_vector);
}
/**
 * do (random_vector*db) and encode result as plaintext
 * @param db_unencoded
 */
void FreivaldsVector::multiply_with_db( std::vector<std::vector<std::uint64_t>> &db_unencoded) {
    auto num_of_rows = pir_params_.nvec[ROW];
    auto num_of_cols = pir_params_.nvec[COL];

    std::vector<std::vector<std::uint64_t>> result_vec;
    result_vec.reserve(num_of_cols);

    // reserve space for multiplication result
    for(uint64_t l = 0; l < num_of_cols; l++){
        std::vector<std::uint64_t> partial_result(enc_params_.poly_modulus_degree(),0);
        result_vec.push_back(partial_result);
    }

    //do multiplication. k row index j col index
    for(uint64_t k = 0; k < num_of_cols; k++) {
        multiply_add(random_vec[0], db_unencoded[k * num_of_rows], result_vec[k]);

        for (uint64_t j = 1; j < num_of_rows; j++) {
            multiply_add(random_vec[j], db_unencoded[j + k * num_of_rows], result_vec[k]);
        }
    }


    std::vector<seal::Plaintext> multiplication_result;
    multiplication_result.reserve(num_of_cols);
    for(uint64_t i = 0; i < num_of_cols; i++){
        seal::Plaintext pt(enc_params_.poly_modulus_degree());
        pt.set_zero();
        encoder_->encode(result_vec[i], pt);
        evaluator_->transform_to_ntt_inplace(pt, context_->first_parms_id());
        multiplication_result.push_back(std::move(pt));
    }

    random_vec_mul_db = std::move(multiplication_result);
}

/**
 * take vector along with constant and do result+= vector*constant
 * @param right vector from above
 * @param left const
 * @param result place to add to
 */
void FreivaldsVector::multiply_add(std::uint64_t left, std::vector<std::uint64_t> &right,
                                   std::vector<std::uint64_t> &result) {
    for(int i=0; i < right.size(); i++){
        result[i]+=(left * right[i]);
    }
}


/**
 * add vector pointwise into second vector
 */
void FreivaldsVector::add_to_sum(std::vector<std::uint64_t> &to_sum,
                                   std::vector<std::uint64_t> &result) {
    for(int i=0; i < to_sum.size(); i++){
        result[i]+=to_sum[i];
    }
}

/**
 * multiply vector with value pointwise
 */
void FreivaldsVector::multiply_with_val(std::uint64_t to_mult_with,std::vector<std::uint64_t> &result) {
    for(int i=0; i < result.size(); i++){
        result[i]*=to_mult_with;
    }
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


    seal::Ciphertext result;

    for(int i=0; i < random_vec.size(); i++){
        seal::Ciphertext temp;
        if(random_vec[i]==1){
            temp = reply[i];
        }
        temp_storage.push_back(std::move(temp));
    }
    evaluator_->add_many(temp_storage, result);
//    seal::Ciphertext difference;
    std::cout<< 11111111111111111<<std::endl;
    evaluator_->sub(random_vec_mul_db_mul_query[query_id], result, response);
    evaluator_->transform_from_ntt_inplace(response);
    if(response.is_transparent()){return true;}
    std::cout<< 11111111111111111<<std::endl;
    return false;
}

//evaluator_->rescale_to_next_inplace(foobar);