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

    std::vector<std::vector<int>> freivalds_vector;
    freivalds_vector.reserve(num_of_rows);

    for (uint64_t k = 0; k < num_of_rows; k++) { // k is smaller than num of rows

        std::vector<int> current_plaintext_multiplier;
        current_plaintext_multiplier.reserve(pir_params_.slot_count);
        generate(current_plaintext_multiplier.begin(), current_plaintext_multiplier.end(), rd()%2);
        freivalds_vector.push_back(std::move(current_plaintext_multiplier));
    }
    this->random_vec = std::move(freivalds_vector);
}

void FreivaldsVector::multiply_with_db(const std::shared_ptr<Database> &db_) {
    auto num_of_rows = pir_params_.nvec[ROW];
    auto num_of_cols = pir_params_.nvec[COL];

    std::vector<seal::Plaintext> *cur = db_.get();
    std::vector<seal::Plaintext> multiplication_result;
    multiplication_result.reserve(num_of_rows);

    for(uint64_t i = 0; i < num_of_rows; i++){
        seal::Plaintext pt(enc_params_.poly_modulus_degree());
        pt.set_zero();
        multiplication_result.push_back(pt);
    }


    for(uint64_t k = 0; k < num_of_rows; k++) { // k is smaller than num of rows
        encoder_->decode()

        for (uint64_t j = 1; j < num_of_cols; j++) {  // j is column

        }

    }



}

