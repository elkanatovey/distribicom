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
void Master::generate_dbase_partition() {
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
//        stringstream current_stream;
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
        current_stream.str(std::string());
        current_stream.clear();
    }
}

std::shared_ptr<Database> Master::get_db(){
    return db_;
}

PirQuerySingleDim Master::get_expanded_query_first_dim_ser(uint32_t client_id, stringstream
                                                          &query_stream) {
    PirQuery query = deserialize_query(query_stream);

    uint64_t dim_to_expand = 0;
    return get_expanded_query_single_dim(client_id, query, dim_to_expand);
}

PirQuerySingleDim
Master::get_expanded_query_single_dim(uint32_t client_id, const PirQuery &query, uint64_t dim_to_expand) {
    vector<Ciphertext> expanded_query;

    uint64_t n_i = pir_params_.nvec[dim_to_expand];
    cout << "Server: n_i = " << n_i << endl;
    cout << "Server: expanding " << query[dim_to_expand].size() << " query ctxts" << endl;
    for (uint32_t j = 0; j < query[dim_to_expand].size(); j++){
        uint64_t total = enc_params_.poly_modulus_degree();
        if (j == query[dim_to_expand].size() - 1){
            total = n_i % enc_params_.poly_modulus_degree();
        }
        cout << "-- expanding one query ctxt into " << total  << " ctxts "<< endl;
        vector<Ciphertext> expanded_query_part = expand_query(query[dim_to_expand][j], total, client_id);
        expanded_query.insert(expanded_query.end(), make_move_iterator(expanded_query_part.begin()),
                              make_move_iterator(expanded_query_part.end()));
        expanded_query_part.clear();
    }
    cout << "Server: expansion done " << endl;
    if (expanded_query.size() != n_i) {
        cout << " size mismatch!!! " << expanded_query.size() << ", " << n_i << endl;
    }

    // Transform expanded query to NTT, and ...
    for (auto & jj : expanded_query) {
        evaluator_->transform_to_ntt_inplace(jj);
    }

    return expanded_query;
}

void Master::set_single_query_second_dim(uint32_t client_id, const PirQuery &query){
    expanded_query_dim2[client_id] = get_expanded_query_single_dim(client_id, query, 1);
}




/**
 *  store queries for redistribution to clients for sharded calculation. Assumes that galois key has been set
 */
void Master::store_query_ser(uint32_t client_id, const string & query)
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
 *  also saves usable version main server side
 */
void Master::store_galois_key_ser(uint32_t client_id, std::stringstream & galois_stream)
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


/**
 * deserialize and store galois key in server
 * @param client_id
 * @param galois_stream
 */
void Master::set_one_galois_key_ser(uint32_t client_id, stringstream &galois_stream) {
    GaloisKeys gkey;
    gkey.load(*context_, galois_stream);
    set_galois_key(client_id, gkey);
}

/**
 *  store queries for redistribution to clients for sharded calculation. Assumes that galois key has been set
 */
void Master::store_query(const PirQuery& query, uint32_t client_id)
{
    uint32_t bucket_id = get_bucket_id(client_id);
    //    std::cout <<"Store Bucket: ....................................." << bucket_id<< std::endl;
    DistributedQueryContext query_context{client_id, query};

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

uint32_t Master::get_bucket_id(uint32_t client_id) {
    auto num_of_rows = getNumberOfPartitions();
    uint32_t bucket_id = client_id / num_of_rows; // first sqrt(n) clients get bucket 0
    return bucket_id;
}

/**
 * return the shard id of relevant row based on worker id
 * @param client_id
 * @return
 */
uint64_t Master::get_row_id(uint32_t client_id)
{
    vector<uint64_t> nvec = pir_params_.nvec;
    auto num_of_rows = nvec[COL];  //if 1d db num of rows is num of ptexts
    if (nvec.size() > 1) {
        num_of_rows = nvec[ROW];
    }

    uint64_t row_id = client_id % num_of_rows;
    return row_id;
}

const DatabaseShard& Master::get_db_row(uint32_t client_id)
{
    uint64_t row_id = get_row_id(client_id);
    return this->db_rows_[row_id];
}

std::string Master::get_db_row_serialized(uint32_t client_id)
{

    uint64_t row_id = get_row_id(client_id); // every cid gets row following that of adjacent cid
    return this->db_rows_serialized_[row_id];
}

uint32_t Master::get_row_len()
{
    return this->db_rows_[0].size();
}





/**
 *  get from server bucket of queries for client to process based on client's ID
 */
DistributedQueryContextBucket Master::get_query_bucket_to_compute(uint32_t client_id)
{
    uint64_t bucket_id = get_bucket_id(client_id);  // first sqrt(n) clients get bucket 0
    return this->query_buckets_[bucket_id];
}

DistributedQueryContextBucketSerial Master::get_query_bucket_to_compute_serialized(uint32_t client_id)
{
    uint32_t bucket_id = get_bucket_id(client_id);
//    std::cout <<"Query Bucket: ....................................." << bucket_id<< std::endl;
    return this->query_buckets_serialized_[bucket_id];
}

/**
 *  get from server bucket of queries for client to process based on client's ID
 */
DistributedGaloisContextBucketSerial Master::get_galois_bucket_ser(uint32_t client_id)
{
    uint64_t bucket_id = get_bucket_id(client_id);  // first sqrt(n) clients get bucket 0
    return this->galois_buckets_serialized_[bucket_id];
}


void Master::process_reply_at_server_ser(std::stringstream & partial_reply_ser, uint32_t client_id) {

    PirReplyShard partial_reply = deserialize_partial_reply(partial_reply_ser);
    process_reply_at_server(client_id, partial_reply);

}

void Master::process_reply_at_server(uint32_t client_id, PirReplyShard &partial_reply) {
    for (auto & i : partial_reply) {
        evaluator_->transform_to_ntt_inplace(i);
    }

    if (partialReplies_.find(client_id) == partialReplies_.end()){
        partialReplies_[client_id] = partial_reply;
    }
    else{
        for(int i=0; i < partialReplies_[client_id].size(); i++){
            evaluator_->add_inplace(partialReplies_[client_id][i], partial_reply[i]);
        }
    }
}



void Master::processQueriesAtServer(const PirReplyShardBucket& queries) {
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

PirReply Master::generateFinalReply(std::uint32_t client_id){
    auto reply = partialReplies_[client_id];

    return reply;
}
int Master::generate_final_reply_ser(std::uint32_t client_id , std::stringstream &stream){
    PirReply reply = generateFinalReply(client_id);
    for (auto & jj : reply) {
        evaluator_->transform_from_ntt_inplace(jj);
        // print intermediate ctxts?
        //cout << "const term of ctxt " << jj << " = " << intermediateCtxts[jj][0] << endl;
    }

   return serialize_reply(reply, stream);
}
std::uint64_t Master::getNumberOfPartitions(){
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
PirReplyShard Master::deserialize_partial_reply(std::stringstream &stream) {
    std::vector< seal::Ciphertext> reply;
    while (stream.rdbuf()->in_avail()) {
        seal::Ciphertext c;
        c.load(*context_, stream);
        reply.push_back(c);
    }
    return reply;
}


PirReply Master::generate_reply_one_dim(PirQuery &query, uint32_t client_id) {

    vector<uint64_t> nvec = pir_params_.nvec;
    uint64_t product = 1;

    for (uint32_t i = 0; i < nvec.size(); i++) {
        product *= nvec[i];
    }

    auto coeff_count = enc_params_.poly_modulus_degree();

    vector<Plaintext> *cur = db_.get();
    vector<Plaintext> intermediate_plain; // decompose....

    auto pool = MemoryManager::GetPool();


    int N = enc_params_.poly_modulus_degree();

    int logt = floor(log2(enc_params_.plain_modulus().value()));

    for (uint32_t i = 0; i < nvec.size(); i++) {
        cout << "Server: " << i + 1 << "-th recursion level started " << endl;


        vector<Ciphertext> expanded_query;

        uint64_t n_i = nvec[i];
        cout << "Server: n_i = " << n_i << endl;
        cout << "Server: expanding " << query[i].size() << " query ctxts" << endl;
        for (uint32_t j = 0; j < query[i].size(); j++){
            uint64_t total = N;
            if (j == query[i].size() - 1){
                total = n_i % N;
            }
            cout << "-- expanding one query ctxt into " << total  << " ctxts "<< endl;
            vector<Ciphertext> expanded_query_part = expand_query(query[i][j], total, client_id);
            expanded_query.insert(expanded_query.end(), std::make_move_iterator(expanded_query_part.begin()),
                                  std::make_move_iterator(expanded_query_part.end()));
            expanded_query_part.clear();
        }
        cout << "Server: expansion done " << endl;
        if (expanded_query.size() != n_i) {
            cout << " size mismatch!!! " << expanded_query.size() << ", " << n_i << endl;
        }

        // Transform expanded query to NTT, and ...
        for (uint32_t jj = 0; jj < expanded_query.size(); jj++) {
            evaluator_->transform_to_ntt_inplace(expanded_query[jj]);
        }

        // Transform plaintext to NTT. If database is pre-processed, can skip
        if ((!is_db_preprocessed_) || i > 0) {
            for (uint32_t jj = 0; jj < cur->size(); jj++) {
                evaluator_->transform_to_ntt_inplace((*cur)[jj], context_->first_parms_id());
            }
        }

        for (uint64_t k = 0; k < product; k++) {
            if ((*cur)[k].is_zero()){
                cout << k + 1 << "/ " << product <<  "-th ptxt = 0 " << endl;
            }
        }

        product /= n_i;

        vector<Ciphertext> intermediateCtxts(product);
        Ciphertext temp;

        for (uint64_t k = 0; k < product; k++) {

            evaluator_->multiply_plain(expanded_query[0], (*cur)[k], intermediateCtxts[k]);

            for (uint64_t j = 1; j < n_i; j++) {
                evaluator_->multiply_plain(expanded_query[j], (*cur)[k + j * product], temp);
                evaluator_->add_inplace(intermediateCtxts[k], temp); // Adds to first component.
            }
        }


        return intermediateCtxts;
        cout << "Server: " << i + 1 << "-th recursion level finished " << endl;
        cout << endl;
    }
    cout << "reply generated!  " << endl;
    // This should never get here
    assert(0);
    vector<Ciphertext> fail(1);
    return fail;
}


PirReply Master::generate_reply_one_dim_enc(PirQuery &query, uint32_t client_id,
                                            std::vector<seal::Ciphertext>* db) {

    vector<uint64_t> nvec = pir_params_.nvec;
    uint64_t product = 1;

    for (uint32_t i = 0; i < nvec.size(); i++) {
        product *= nvec[i];
    }

    vector<Ciphertext> *cur = db;

    auto pool = MemoryManager::GetPool();


    int N = enc_params_.poly_modulus_degree();


    for (uint32_t i = 0; i < nvec.size(); i++) {
        cout << "Server: " << i + 1 << "-th recursion level started " << endl;


        vector<Ciphertext> expanded_query;

        uint64_t n_i = nvec[i];
        cout << "Server: n_i = " << n_i << endl;
        cout << "Server: expanding " << query[i].size() << " query ctxts" << endl;
        for (uint32_t j = 0; j < query[i].size(); j++){
            uint64_t total = N;
            if (j == query[i].size() - 1){
                total = n_i % N;
            }
            cout << "-- expanding one query ctxt into " << total  << " ctxts "<< endl;
            vector<Ciphertext> expanded_query_part = expand_query(query[i][j], total, client_id);
            expanded_query.insert(expanded_query.end(), std::make_move_iterator(expanded_query_part.begin()),
                                  std::make_move_iterator(expanded_query_part.end()));
            expanded_query_part.clear();
        }
        cout << "Server: expansion done " << endl;
        if (expanded_query.size() != n_i) {
            cout << " size mismatch!!! " << expanded_query.size() << ", " << n_i << endl;
        }


        product /= n_i;

        vector<Ciphertext> intermediateCtxts(product);
        Ciphertext temp;

        for (uint64_t k = 0; k < product; k++) {

            evaluator_->multiply(expanded_query[0], (*cur)[k], intermediateCtxts[k]);

            for (uint64_t j = 1; j < n_i; j++) {
                evaluator_->multiply(expanded_query[j], (*cur)[k + j * product], temp);
                evaluator_->add_inplace(intermediateCtxts[k], temp); // Adds to first component.
            }
        }


        return intermediateCtxts;
        cout << "Server: " << i + 1 << "-th recursion level finished " << endl;
        cout << endl;
    }
    cout << "reply generated!  " << endl;
    // This should never get here
    assert(0);
    vector<Ciphertext> fail(1);
    return fail;
}

Master::Master(const seal::EncryptionParameters& parameters, const PirParams& params): PIRServer(parameters, params) {}
