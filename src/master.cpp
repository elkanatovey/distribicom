#include "master.hpp"

#define COL 0
#define ROW 1

using namespace std;
using namespace seal;
using namespace seal::util;


/**
 * create row partition of db for distributing work - both serialised version and not serialsed
 * version
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
        (*cur)[k].save(current_stream);

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

void MasterServer::db_to_vec(std::vector<std::vector<std::uint64_t>> &db_unencoded){
    auto *db = db_.get();

    for(int i=0; i<db->size();i++){
        encoder_->decode((*db)[i], db_unencoded[i]);
    }
}





/**
 *  store queries for redistribution to clients for sharded calculation. Assumes that galois key has been set
 */
void MasterServer::store_query_ser(uint32_t client_id, const string & query)
{
    uint32_t bucket_id = get_bucket_id(client_id);
//    std::cout <<"Store Bucket: ....................................." << bucket_id<< std::endl;

    DistributedQueryContextSerial query_context_serial{client_id, query};


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
 *  store galois_stream keys for redistribution to clients for sharded calculation and for local calculation.
 */
void MasterServer::store_galois_key_ser(uint32_t client_id, std::stringstream & galois_stream)
{
    uint32_t bucket_id = get_bucket_id(client_id);
    //    std::cout <<"Store Bucket: ....................................." << bucket_id<< std::endl;

    DistributedGaloisContextSerial galois_context_serial{client_id, galois_stream.str()};
    set_one_galois_key_ser(client_id, galois_stream);

    // no query inserted to bucket yet case
    if (this->galois_buckets_serialized_.find(bucket_id) == this->galois_buckets_serialized_.end())
    {
        vector<DistributedGaloisContextSerial> current_bucket_serial;
        current_bucket_serial.push_back(galois_context_serial);
        this->galois_buckets_serialized_[bucket_id] = move(current_bucket_serial);
    }
    else{
        this->galois_buckets_serialized_[bucket_id].push_back(galois_context_serial);
    }
}

void MasterServer::set_one_galois_key_ser(uint32_t client_id, stringstream &galois_stream) {
    GaloisKeys gkey;
    gkey.load(*context_, galois_stream);
    set_galois_key(client_id, gkey);
}

/**
 *  store queries for redistribution to clients for sharded calculation. Assumes that galois key has been set
 */
void MasterServer::store_query(const PirQuery& query, uint32_t client_id)
{
    vector<uint64_t> nvec = pir_params_.nvec;
    uint32_t bucket_id = get_bucket_id(client_id);
    //    std::cout <<"Store Bucket: ....................................." << bucket_id<< std::endl;
    DistributedQueryContext query_context{client_id, query};

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

uint32_t MasterServer::get_bucket_id(uint32_t client_id) {
    auto num_of_rows = getNumberOfPartitions();
    uint32_t bucket_id = client_id / num_of_rows; // first sqrt(n) clients get bucket 0
    return bucket_id;
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
 *  get from server bucket of queries for client to process based on client's ID
 */
DistributedQueryContextBucket MasterServer::get_query_bucket_to_compute(uint32_t client_id)
{
    uint64_t bucket_id = get_bucket_id(client_id);  // first sqrt(n) clients get bucket 0
    return this->query_buckets_[bucket_id];
}

DistributedQueryContextBucketSerial MasterServer::get_query_bucket_to_compute_serialized(uint32_t client_id)
{
    uint32_t bucket_id = get_bucket_id(client_id);
//    std::cout <<"Query Bucket: ....................................." << bucket_id<< std::endl;
    return this->query_buckets_serialized_[bucket_id];
}

/**
 *  get from server bucket of queries for client to process based on client's ID
 */
DistributedGaloisContextBucketSerial MasterServer::get_galois_bucket_ser(uint32_t client_id)
{
    uint64_t bucket_id = get_bucket_id(client_id);  // first sqrt(n) clients get bucket 0
    return this->galois_buckets_serialized_[bucket_id];
}


void MasterServer::process_reply_at_server_ser(std::stringstream & partial_reply_ser, uint32_t client_id) {

    PirReplyShard partial_reply = deserialize_partial_reply(partial_reply_ser);
    for (auto & i : partial_reply) {
        evaluator_->transform_to_ntt_inplace(i);
    }

    if (partialReplies_.find(client_id) == partialReplies_.end()){
        partialReplies_[client_id] = partial_reply;
    }
    else{
        for(int i=0; i<partialReplies_[client_id].size(); i++){
            evaluator_->add_inplace(partialReplies_[client_id][i], partial_reply[i]);
        }
    }
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
int MasterServer::generate_final_reply_ser(std::uint32_t client_id , std::stringstream &stream){
    PirReply reply = generateFinalReply(client_id);
   return serialize_reply(reply, stream);
}
std::uint64_t MasterServer::getNumberOfPartitions(){
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    return num_of_rows;
}

/**
 * This function assumes that the only thing in the buffer is the partial reply.
 * @todo possibly change later
 */
PirReplyShard MasterServer::deserialize_partial_reply(std::stringstream &stream) {
    std::vector< seal::Ciphertext> reply;
    while (stream.rdbuf()->in_avail()) {
        seal::Ciphertext c;
        c.load(*context_, stream);
        reply.push_back(c);
    }
    return reply;
}

MasterServer::MasterServer(const seal::EncryptionParameters& parameters, const PirParams& params): PIRServer(parameters, params) {}
