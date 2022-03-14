#include "master.hpp"

#define COL 0
#define ROW 1

using namespace std;
using namespace seal;
using namespace seal::util;


/**
 * create row partition of db for distibuting work
 */
void MasterServer::generate_dbase_partition() {
    vector<uint64_t> nvec = pir_params_.nvec;

    unsigned long num_of_cols = 1;
    auto num_of_rows = nvec[COL];
    if (nvec.size() > 1) {
        num_of_cols = nvec[COL];
        num_of_rows = nvec[ROW];
    }

    vector<seal::Plaintext> *cur = db_.get();  // current left matrix
    vector<seal::Plaintext> intermediate_plain; // intermediate ctexts will be decomposed and mapped to here in second level
    stringstream current_stream;

    for (uint64_t k = 0; k < num_of_rows; k++) { // k is smaller than num of rows

        vector<seal::Plaintext> current_row;
        current_row.reserve(num_of_cols);
        current_row.push_back((*cur)[k]);

        for (uint64_t j = 1; j < num_of_cols; j++) {  // j is column
            current_row.push_back((*cur)[k + j * num_of_rows]);
            (*cur)[k + j * num_of_rows].save(current_stream);
            }
        string row = current_stream.str();
        this->db_rows_serialized_[k] = move(row);
        this->db_rows_[k] = move(current_row);
        current_stream.clear();
    }
}

/**
 *  store queries for redistribution to clients for sharded calculation. Assumes that galois key has been set
 */
void MasterServer::store_query_ser(uint32_t client_id, uint32_t count, const string & query, const string & galois_key)
{
    auto num_of_rows = this->getNumberOfPartitions();
    uint32_t bucket_id = client_id / num_of_rows;
//    std::cout <<"Store Bucket: ....................................." << bucket_id<< std::endl;

    DistributedQueryContextSerial query_context_serial{client_id, query, galois_key};


    // no query inserted to bucket yet case
    if (this->query_buckets_serialized_.find(bucket_id) == this->query_buckets_serialized_.end())
    {
        vector<DistributedQueryContextSerial> current_bucket_serial;
        current_bucket_serial.push_back(query_context_serial);
        this->query_buckets_serialized_[bucket_id] = move(current_bucket_serial);
    }
    else{
        this->query_buckets_serialized_[bucket_id].push_back(query_context_serial);
    }
}


/**
 *  store queries for redistribution to clients for sharded calculation. Assumes that galois key has been set
 */
void MasterServer::store_query(const PirQuery& query, uint32_t client_id)
{
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = this->getNumberOfPartitions();
    uint32_t bucket_id = client_id / num_of_rows;
//    std::cout <<"Store Bucket: ....................................." << bucket_id<< std::endl;
    DistributedQueryContext query_context{client_id, query, this->galoisKeys_[client_id]};

    uint32_t count = query[0].size(); //@todo verify this

    // no query inserted to bucket yet case
    if (this->query_buckets_.find(bucket_id) == this->query_buckets_.end())
    {
        vector<DistributedQueryContext> current_bucket;
        current_bucket.push_back(query_context);
        this->query_buckets_[bucket_id] = move(current_bucket);

    }
    else{
        this->query_buckets_[bucket_id].push_back(query_context);
    }
}

uint64_t MasterServer::get_row_id(uint32_t client_id)
{
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    uint64_t row_id = client_id % num_of_rows;
    return row_id;
}

const DatabaseShard& MasterServer::get_db_row(uint32_t client_id)
{
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    uint64_t row_id = client_id % num_of_rows;
    return this->db_rows_[row_id];
}

std::string MasterServer::get_db_row_serialized(uint32_t client_id)
{
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    uint64_t row_id = client_id % num_of_rows; // every cid gets row following that of adjacent cid
    return this->db_rows_serialized_[row_id];
}

uint32_t MasterServer::get_row_len()
{
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    return this->db_rows_[0].size();
}





/**
 *  get from server bucket of queries foir client to process based on client's ID
 */
DistributedQueryContextBucket MasterServer::get_query_bucket_to_compute(uint32_t client_id)
{
    auto num_of_rows = getNumberOfPartitions();  //if 1d db num of rows is num of ptexts

    uint64_t bucket_id = client_id / num_of_rows; // first sqrt(n) clients get bucket 0
    return this->query_buckets_[bucket_id];
}

DistributedQueryContextBucketSerial MasterServer::get_query_bucket_to_compute_serialized(uint32_t client_id)
{
    auto num_of_rows = getNumberOfPartitions();;  //if 1d db num of rows is num of ptexts
    uint64_t bucket_id = client_id / num_of_rows;
//    std::cout <<"Query Bucket: ....................................." << bucket_id<< std::endl;
    return this->query_buckets_serialized_[bucket_id];
}

void MasterServer::processQueriesAtServer(const PirReplyShardBucket& queries) {
    for (auto const& [clientID, partialReply] : queries){
        if (partialReplies_.find(clientID) == partialReplies_.end()){
            partialReplies_[clientID] = partialReply;
        }
        else{
            for(int i=0; i<partialReplies_[clientID].size(); i++){
                evaluator_->add_inplace(partialReplies_[clientID][i], partialReply[i]);
            }
        }
    }
}

PirReply MasterServer::generateFinalReply(std::uint32_t client_id){
    auto reply = partialReplies_[client_id];

    for (auto & i : reply) {
        evaluator_->transform_from_ntt_inplace(i);
    }
    return reply;
}
std::uint64_t MasterServer::getNumberOfPartitions(){
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    return num_of_rows;
}

MasterServer::MasterServer(const seal::EncryptionParameters& parameters, const PirParams& params): PIRServer(parameters, params) {}
