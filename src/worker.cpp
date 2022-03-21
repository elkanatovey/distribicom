#include "worker.hpp"

using namespace std;
using namespace seal;
using namespace seal::util;

/**
 *  Note for now: local server for client constructor
 */
ClientSideServer::ClientSideServer(const seal::EncryptionParameters &seal_params, const PirParams &pir_params, unique_ptr<vector<seal::Plaintext>> db, uint32_t shard_id ) :
        PIRServer(seal_params, pir_params)
{
    db_ = move(db);
    is_db_preprocessed_ = false;
    shard_id_ = shard_id;
}


/**
 *  Note for now: local server for client constructor
 */
ClientSideServer::ClientSideServer(const seal::EncryptionParameters &seal_params, const PirParams
&pir_params, std::string db, uint32_t shard_id,uint32_t row_len) :
PIRServer(seal_params, pir_params)
{
    db_ = make_unique<vector<seal::Plaintext>>();
    stringstream temp_db_;
    temp_db_ << db;
    for (uint32_t j = 0; j < row_len; j++) {
        Plaintext p;
        p.load(*context_, temp_db_);
        db_->push_back(p);
    }

    is_db_preprocessed_ = false;
    shard_id_ = shard_id;
}


void ClientSideServer::set_one_galois_key_ser(uint32_t client_id, stringstream &galois_stream) {
    GaloisKeys gkey;
    gkey.load(*context_, galois_stream);
    set_galois_key(client_id, gkey);
}

/**
 *  get partial answers from a bucket
 */
PirReplyShardBucket ClientSideServer::processQueryBucketAtClient(DistributedQueryContextBucket
queries){ // @todo parallelize
    PirReplyShardBucket answers;
    for(const auto& query_context: queries){
        PirReplyShard reply = processQueryAtClient(query_context.query, query_context.client_id);
        answers[query_context.client_id] = reply;
    }
    return answers;
}

/**
 *  get partial answers from a bucket
 */
PirReplyShardBucketSerial ClientSideServer::process_query_bucket_at_client_ser_
(DistributedQueryContextBucketSerial queries){ // @todo parallelize
    PirReplyShardBucketSerial answers;
    stringstream query_stream;
    stringstream partial_reply_stream;
    for(const auto& query_context: queries){
        query_stream.str(query_context.query);
        process_query_at_client_ser(query_stream,partial_reply_stream , query_context.client_id);

        answers[query_context.client_id] = partial_reply_stream.str();
        query_stream.clear();
        partial_reply_stream.clear();
    }
    return answers;
}

int ClientSideServer::process_query_at_client_ser(stringstream &query_stream, stringstream&
                                                                  reply_stream,uint32_t client_id){
    PirQuery current_query = deserialize_query(query_stream);
    query_stream.clear();
    PirReplyShard reply = processQueryAtClient(current_query, client_id);
    auto num_bytes = serialize_reply(reply, reply_stream);
    return num_bytes;
}

/**
 *  do matrix multiplication with local shard, returns local part of answer  --assumes db is 2dim and galois key inserted
 */
PirReplyShard ClientSideServer::processQueryAtClient(PirQuery query, uint32_t client_id) {

    vector<uint64_t> nvec = pir_params_.nvec;
    uint64_t product = nvec[0]; //don't count rows in calculation, as only one row in shard @todo check the math of this
//    for (unsigned long i : nvec) {product *= i;}

//    auto coeff_count = enc_params_.poly_modulus_degree();
    vector<Plaintext> *cur = db_.get();  // current left matrix
    vector<Plaintext> intermediate_plain; // intermediate ctexts will be decomposed and mapped to here in second level
    auto pool = MemoryManager::GetPool();


//    int logt = floor(log2(enc_params_.plain_modulus().value()));
//    cout << "expansion ratio = " << pir_params_.expansion_ratio << endl;

    // actual multiplications happen in this loop
    for (uint32_t i = 0; i < nvec.size(); i++) {  // for dim in dims
        cout << "Server: " << i + 1 << "-th recursion level started " << endl;

        vector<Ciphertext> expanded_query;

        uint64_t n_i = nvec[i];
        expand_query_single_dim(expanded_query,  n_i, query[i], client_id);

        // Transform plaintext to NTT. If database is pre-processed, can skip
        if ((!is_db_preprocessed_) || i > 0) {
            is_db_preprocessed_ = true;
            for (uint32_t jj = 0; jj < cur->size(); jj++) {
                evaluator_->transform_to_ntt_inplace((*cur)[jj], context_->first_parms_id());
            }
        }

        for (uint64_t k = 0; k < product; k++) {
            if ((*cur)[k].is_zero()){
                cout << k + 1 << "/ " << product <<  "-th ptxt = 0 " << endl;
            }
        }

        if(i == 0)  // i==0 -->get rid of col count, i==1--> we already didn't count rows so don't erase again
        {product /= n_i;}

        vector<Ciphertext> intermediateCtxts(product); // first round size is 1 - aka num of rows in local db
        Ciphertext temp;
        if(i == 0) {
            doPirMultiplication(product, cur, expanded_query, n_i, intermediateCtxts, temp);

        }
        else
        {
            for (uint64_t k = 0; k < product; k++) {
                evaluator_->multiply_plain(expanded_query[this->shard_id_], (*cur)[k], intermediateCtxts[k]); //skip members of query vector according to row that we calculated from db
            }
        };
        for (uint32_t jj = 0; jj < intermediateCtxts.size(); jj++) {
            evaluator_->transform_from_ntt_inplace(intermediateCtxts[jj]);
            // print intermediate ctxts?
            //cout << "const term of ctxt " << jj << " = " << intermediateCtxts[jj][0] << endl;
        }

        if (i == nvec.size() - 1) {
            return intermediateCtxts;
        } else { // current left matrix is updated here
            turn_intermediateCtexts_to_db_format( intermediate_plain,  intermediateCtxts,
                                                 product, cur);

        }
        cout << "Server: " << i + 1 << "-th recursion level finished " << endl;
        cout << endl;
    }
    cout << "reply generated!  " << endl;
    // This should never get here
    assert(0);
    vector<Ciphertext> fail(1);
    return fail;
}

inline void ClientSideServer::expand_query_single_dim(vector<Ciphertext> &expanded_query, uint64_t n_i,  // n_i
                                       // is current dim
                                               const PirQuerySingleDim& query, uint32_t client_id){
    auto N = enc_params_.poly_modulus_degree();
    cout << "Server: n_i = " << n_i << endl;
    cout << "Server: expanding " << query.size() << " query ctxts" << endl;
    for (uint32_t j = 0; j < query.size(); j++){
        uint64_t total = N;
        if (j == query.size() - 1){
            total = n_i % N;
        }
        cout << "-- expanding one query ctxt into " << total  << " ctxts "<< endl;
        vector<Ciphertext> expanded_query_part = expand_query(query[j], total, client_id);
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
}

/**
 *  do the multiplication of the query by db for current dimension
 */
inline void ClientSideServer::doPirMultiplication(uint64_t product, const vector<Plaintext> *cur, const
vector<Ciphertext> &expanded_query,
                                           uint64_t n_i, vector<Ciphertext> &intermediateCtxts, Ciphertext &temp) {
    for (uint64_t k = 0; k < product; k++) { // k is row in db

        // multiply query[0] by first column
        evaluator_->multiply_plain(expanded_query[0], (*cur)[k], intermediateCtxts[k]); //db[k][0]*q[0]

        for (uint64_t j = 1; j < n_i; j++) {  // j is column in db
            evaluator_->multiply_plain(expanded_query[j], (*cur)[k + j * product], temp);
            evaluator_->add_inplace(intermediateCtxts[k], temp); // Adds to first component.
        }
    }
}

/**
 *  map intermediateCtxts to vector of plaintexts (intermediate_plain), and update curr (the db pointer) to point
 *  to intermediate_plain. This will be treated as the current db in the next iteration of multiplications
 */
inline void ClientSideServer::turn_intermediateCtexts_to_db_format( vector<Plaintext>
        &intermediate_plain, vector<Ciphertext> &intermediateCtxts,
                                                            uint64_t &product, vector<Plaintext> *&cur) {
    intermediate_plain.clear();
    intermediate_plain.reserve(pir_params_.expansion_ratio * product);
    cur = &intermediate_plain;


    for (uint64_t rr = 0; rr < product; rr++) {
        EncryptionParameters parms;
        if(pir_params_.enable_mswitching){
            evaluator_->mod_switch_to_inplace(intermediateCtxts[rr], context_->last_parms_id());
            parms = context_->last_context_data()->parms();
        }
        else{
            parms = context_->first_context_data()->parms();
        }
        vector<Plaintext> plains = decompose_to_plaintexts(parms,
                                                           intermediateCtxts[rr]);

        for (uint32_t jj = 0; jj < plains.size(); jj++) {
            intermediate_plain.emplace_back(plains[jj]);
        }
    }
    product = intermediate_plain.size(); // multiply by expansion rate.
}