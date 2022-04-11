#pragma once


#include "pir_server.hpp"
#include "distributed_pir.hpp"

class MasterServer : public PIRServer {

    std::map<std::uint64_t, DatabaseShard> db_rows_;
    std::map<std::uint64_t, std::string> db_rows_serialized_;

    std::map<std::uint64_t, DistributedQueryContextBucket> query_buckets_;
    std::map<std::uint64_t, DistributedQueryContextBucketSerial> query_buckets_serialized_;

    std::map<std::uint64_t, DistributedGaloisContextBucketSerial> galois_buckets_serialized_;
    std::map<ClientID , PirReplyShard> partialReplies_;

    std::map<ClientID , PirQuerySingleDim> expanded_query_dim2;  //expanded dim 2 of query
    uint64_t ptext_len_;


public:
    MasterServer(const seal::EncryptionParameters& parameters, const PirParams& params);

// distributed pir functions server side
void store_query(const PirQuery& query, uint32_t client_id);

    const DatabaseShard &get_db_row(uint32_t client_id);

    DistributedQueryContextBucket get_query_bucket_to_compute(uint32_t client_id);

    void processQueriesAtServer(const PirReplyShardBucket& queries);

    PirReply generateFinalReply(std::uint32_t client_id);

    // server side pir computations
    void generate_dbase_partition();

    uint64_t get_row_id(uint32_t client_id);

    string get_db_row_serialized(uint32_t client_id);

    uint32_t get_row_len();


    DistributedQueryContextBucketSerial get_query_bucket_to_compute_serialized(uint32_t client_id);

    uint64_t getNumberOfPartitions();

    void store_query_ser(uint32_t client_id, const string &query);

    void store_galois_key_ser(uint32_t client_id, std::stringstream &galois_stream);

    int generate_final_reply_ser(uint32_t client_id, stringstream &stream);

    PirReplyShard deserialize_partial_reply(stringstream &stream);

    void process_reply_at_server_ser(stringstream &partial_reply_ser, uint32_t client_id);

    uint32_t get_bucket_id(uint32_t client_id);

    DistributedGaloisContextBucketSerial get_galois_bucket_ser(uint32_t client_id);

    void set_one_galois_key_ser(uint32_t client_id, stringstream &galois_stream);

    void db_to_vec(vector<std::vector<std::uint64_t>> &db_unencoded);

    PirQuerySingleDim get_expanded_query_first_dim_ser(uint32_t client_id, stringstream &query_stream);

    void process_reply_at_server(uint32_t client_id, PirReplyShard &partial_reply);

    PirQuerySingleDim get_expanded_query_single_dim(uint32_t client_id, const PirQuery &query, uint64_t dim_to_expand);

    void set_single_query_second_dim(uint32_t client_id, const PirQuery &query);
};

