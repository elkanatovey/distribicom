#pragma once


#include "pir_server.hpp"
#include "distributed_pir.hpp"

class ClientSideServer : public PIRServer {

    std::uint32_t shard_id_;

// client side pir computations
PirReplyShard processQueryAtClient(PirQuery query, uint32_t client_id, bool do_second_level=true);

public:
// constructor for client side server
ClientSideServer(const seal::EncryptionParameters &seal_params, const PirParams &pir_params,
                 unique_ptr<vector<seal::Plaintext>>db, uint32_t shard_id);
ClientSideServer(const seal::EncryptionParameters &seal_params, const PirParams& pir_params,
                 std::string db, uint32_t shard_id,uint32_t row_len);

// distributed pir functions client side
PirReplyShardBucket processQueryBucketAtClient(DistributedQueryContextBucket queries);

PirReplyShardBucketSerial process_query_bucket_at_client_ser_(DistributedQueryContextBucketSerial
queries);

/**
 *  do_second_level decides if worker does partial pir also on second level of recursion
 */
int process_query_at_client_ser(stringstream &query_stream, stringstream &reply_stream, uint32_t client_id, bool do_second_level=true);

void set_one_galois_key_ser(uint32_t client_id, stringstream &galois_stream);

private:
    void expand_query_single_dim(vector<seal::Ciphertext> &expanded_query, std::uint64_t n_i,
                                 const PirQuerySingleDim& query, std::uint32_t client_id);

    void doPirMultiplication(uint64_t product, const vector<seal::Plaintext> *cur,
                             const vector<seal::Ciphertext> &expanded_query, uint64_t n_i,
                             vector<seal::Ciphertext> &intermediateCtxts, seal::Ciphertext &temp);

    void turn_intermediateCtexts_to_db_format( Database &intermediate_plain,
                                               vector<seal::Ciphertext> &intermediateCtxts,
                                               uint64_t &product, Database *&cur);


};


