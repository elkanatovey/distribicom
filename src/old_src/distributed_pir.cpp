#include "distributed_pir.hpp"


void deserialize_db(const seal::SEALContext &context_, std::stringstream &temp_db_, uint32_t row_len,
                    std::shared_ptr<std::vector<seal::Plaintext>> &db_) {
    for (uint32_t j = 0; j < row_len; j++) {
        seal::Plaintext p;
        p.load(context_, temp_db_);
        db_->push_back(p);
    }
}