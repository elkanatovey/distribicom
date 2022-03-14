#pragma once
#include "pir.hpp"

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