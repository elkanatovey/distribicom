#pragma once


#include "pir_server.hpp"
#include "distributed_pir.hpp"

class MasterServer : public PIRServer {

    std::map<std::uint64_t, DatabaseShard> db_rows_;
    std::map<std::uint64_t, std::string> db_rows_serialized_;
    std::map<int, std::stringstream> galoisKeys_serialised_;

    std::map<std::uint64_t, DistributedQueryContextBucket> query_buckets_;
    std::map<std::uint64_t, DistributedQueryContextBucketSerial> query_buckets_serialized_;

    std::map<std::uint64_t, DistributedGaloisContextBucketSerial> galois_buckets_serialized_;
    std::map<ClientID , PirReplyShard> partialReplies_;
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

    void store_galois_key_ser(uint32_t client_id, string &galois);

    void generate_final_reply_ser(uint32_t client_id, stringstream &stream);
};

