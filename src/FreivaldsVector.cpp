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

    std::vector<std::vector<std::uint64_t>> freivalds_vector;
    freivalds_vector.reserve(num_of_rows);

    for (uint64_t k = 0; k < num_of_rows; k++) { // k is smaller than num of rows

        std::vector<std::uint64_t> current_plaintext_multiplier(pir_params_.slot_count);
        generate(current_plaintext_multiplier.begin(), current_plaintext_multiplier.end(), [&gen](){return gen->generate(); });
        freivalds_vector.push_back(std::move(current_plaintext_multiplier));
    }
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

void FreivaldsVector::multiply_add(std::vector<std::uint64_t>& left, std::vector<std::uint64_t>& right,
                               std::vector<std::uint64_t>& result){
    for(int i=0;i< left.size(); i++){
        result[i]+=(left[i] * right[i]);
    }
}

void FreivaldsVector::multiply_with_query(uint32_t client_id, const std::vector<seal::Ciphertext>& query){

    seal::Ciphertext temp;

    seal::Ciphertext result;
    evaluator_->multiply_plain(query[0],random_vec_mul_db[0] , result);

    for(int i=1; i<random_vec_mul_db.size(); i++){
        evaluator_->multiply_plain(query[i],random_vec_mul_db[i] , temp);
        evaluator_->add_inplace(result, temp);
    }

    random_vec_mul_db_mul_query[client_id] = result;
}

bool FreivaldsVector::multiply_with_reply(uint32_t client_id, uint32_t db_shard_id,
                                                  const std::vector<seal::Ciphertext> &reply) {
    seal::Ciphertext temp;

    seal::Ciphertext result;
    evaluator_->multiply_plain(reply[0],random_vec[0] , result);
    for(int i=0; i<random_vec.size(); i++){
        evaluator_->multiply_plain(reply[i],random_vec_mul_db[i] , temp);
        evaluator_->add_inplace(result, temp);
    }
//        random_vec_mul_db_mul_query[client_id] - result;
//@todo fix this so freivalds vector works on full plaintexts
    return false;
}

