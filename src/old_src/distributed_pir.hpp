#pragma once
#include "pir.hpp"
#define COL 0
#define ROW 1

typedef std::vector<seal::Ciphertext> PirQuerySingleDim; // one dimension of a PirQuery

struct DistributedQueryContext {
    std::uint32_t client_id;
    const PirQuery query;                 // query
};

struct DistributedQueryContextSerial {
    std::uint32_t client_id;
    std::string query;                 // query
};

struct DistributedGaloisContextSerial {
    std::uint32_t client_id;
    std::string galois;                 // galois key
};

typedef std::vector<seal::Plaintext> DatabaseShard;
typedef std::uint64_t ClientID;
typedef std::vector<seal::Ciphertext> PirReplyShard;
typedef std::map<ClientID, PirReplyShard> PirReplyShardBucket;

typedef std::string PirReplyShardSerial;
typedef std::map<ClientID, PirReplyShardSerial> PirReplyShardBucketSerial;

typedef std::vector<DistributedQueryContext> DistributedQueryContextBucket; // store pirQueries to pass to workers

typedef std::vector<DistributedQueryContextSerial> DistributedQueryContextBucketSerial;

typedef std::vector<DistributedGaloisContextSerial> DistributedGaloisContextBucketSerial;

struct RegistrationParams {
    uint64_t number_of_items;
    uint64_t size_per_item;
    uint32_t N;
    uint32_t logt;
    uint32_t d;
};

void deserialize_db(const seal::SEALContext &context_, std::stringstream &temp_db_, uint32_t row_len,
                    std::shared_ptr<std::vector<seal::Plaintext>> &db_);