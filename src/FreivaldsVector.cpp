#include "FreivaldsVector.hpp"

#define COL 0
#define ROW 1

FreivaldsVector::FreivaldsVector(const seal::EncryptionParameters &enc_params, const PirParams& pir_params):enc_params_(enc_params),
pir_params_(pir_params){  //generates the freeivalds vec

    context_ = std::make_shared<seal::SEALContext>(enc_params, true);
    evaluator_ = std::make_unique<seal::Evaluator>(*context_);
    encoder_ = std::make_unique<seal::BatchEncoder>(*context_);

    auto num_of_rows = pir_params_.nvec[ROW];

    std::random_device rd; // @todo this needs to be implemented with a cprng
    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();

    std::vector<std::uint64_t> freivalds_vector(num_of_rows);
    generate(freivalds_vector.begin(), freivalds_vector.end(), [&gen](){return gen->generate()%2; });

    this->random_vec = std::move(freivalds_vector);
}

void FreivaldsVector::multiply_with_db( std::vector<std::vector<std::uint64_t>> &db_unencoded) {
    auto num_of_rows = pir_params_.nvec[ROW];
    auto num_of_cols = pir_params_.nvec[COL];

    std::vector<std::vector<std::uint64_t>> result_vec;
    result_vec.reserve(num_of_cols);

    for(uint64_t l = 0; l < num_of_cols; l++){
        std::vector<std::uint64_t> partial_result(enc_params_.poly_modulus_degree());
        result_vec.push_back(partial_result);
    }


    for(uint64_t k = 0; k < num_of_rows; k++) { // k is smaller than num of rows
        multiply_add(db_unencoded[k], random_vec[0], result_vec[0]);

        for (uint64_t j = 1; j < num_of_cols; j++) {  // j is column
            multiply_add(db_unencoded[k + j * num_of_rows], random_vec[j], result_vec[j]);
        }
    }

    std::vector<seal::Plaintext> multiplication_result;
    multiplication_result.reserve(num_of_rows);
    for(uint64_t i = 0; i < num_of_rows; i++){
        seal::Plaintext pt(enc_params_.poly_modulus_degree());
        pt.set_zero();
        encoder_->encode(result_vec[i], pt);
        evaluator_->transform_to_ntt_inplace(pt, context_->first_parms_id());
        multiplication_result.push_back(pt);
    }

    random_vec_mul_db = multiplication_result;
}

/**
 * take vector along with constant and do result+= vector*constant
 * @param left vector from above
 * @param right const
 * @param result place to add to
 */
void FreivaldsVector::multiply_add(std::vector<std::uint64_t>& left, std::uint64_t right,
                               std::vector<std::uint64_t>& result){
    for(int i=0;i< left.size(); i++){
        result[i]+=(left[i] * right);
    }
}


/**
 * execute (freivalds_vec*db)*query with an expanded query
 * @param client_id
 * @param query
 */
void FreivaldsVector::multiply_with_query(uint32_t client_id, const std::vector<seal::Ciphertext>& query){
    random_vec_mul_db_mul_query[client_id].reserve(random_vec_mul_db.size());
    for(int i=0; i<random_vec_mul_db.size(); i++){
        seal::Ciphertext temp;
        evaluator_->multiply_plain(query[i],random_vec_mul_db[i] , temp);
        random_vec_mul_db_mul_query[client_id].push_back(std::move(temp));
    }
}

/**
 * execute freivalds_vec*(db*query) for single db shard. This essentially checks single ciphertext
 * @param query_id
 * @param db_shard_id
 * @param reply
 * @return
 */
bool FreivaldsVector::multiply_with_reply(uint32_t query_id, uint32_t db_shard_id,
                                                  const std::string &foo) {
    std:std::stringstream s(foo);
    seal::Ciphertext reply;
    reply.load(*context_, s);

    seal::Ciphertext result;

    if(random_vec[db_shard_id]==1){ // multiplication with 1 in base case
        result = reply;
    }

    seal::Ciphertext temp;
    evaluator_->sub(random_vec_mul_db_mul_query[query_id][db_shard_id], result, temp);

    if(temp.is_transparent()){return true;}
    std::cout<< 11111111111111111<<std::endl;
    return false;
}

