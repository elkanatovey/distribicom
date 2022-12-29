#pragma once


#include "pir_server.hpp"
#include "distributed_pir.hpp"

class Worker : public PIRServer {

    std::uint32_t shard_id_;

// client side pir computations
PirReplyShard processQueryAtClient(PirQuery query, uint32_t client_id, bool do_second_level=true);

public:
// constructor for client side server
Worker(const seal::EncryptionParameters &seal_params, const PirParams &pir_params,
       std::unique_ptr<std::vector<seal::Plaintext>>db, uint32_t shard_id);
Worker(const seal::EncryptionParameters &seal_params, const PirParams& pir_params,
       std::stringstream &db_stream, uint32_t shard_id, uint32_t row_len);

// distributed pir functions client side
PirReplyShardBucket processQueryBucketAtClient(DistributedQueryContextBucket queries);

PirReplyShardBucketSerial process_query_bucket_at_client_ser_(DistributedQueryContextBucketSerial
queries);

/**
 *  do_second_level decides if worker does partial pir also on second level of recursion
 */
int process_query_at_client_ser(std::stringstream &query_stream, std::stringstream &reply_stream, uint32_t client_id, bool do_second_level=true);

void set_one_galois_key_ser(uint32_t client_id, std::stringstream &galois_stream);

private:
    void expand_query_single_dim(std::vector<seal::Ciphertext> &expanded_query, std::uint64_t n_i,
                                 const PirQuerySingleDim& query, std::uint32_t client_id);

    void doPirMultiplication(uint64_t product, const std::vector<seal::Plaintext> *cur,
                             const std::vector<seal::Ciphertext> &expanded_query, uint64_t n_i,
                             std::vector<seal::Ciphertext> &intermediateCtxts, seal::Ciphertext &temp);

    void turn_intermediateCtexts_to_db_format( Database &intermediate_plain,
                                               std::vector<seal::Ciphertext> &intermediateCtxts,
                                               uint64_t &product, Database *&cur);


};


