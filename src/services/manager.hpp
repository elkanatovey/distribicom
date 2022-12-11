#pragma once

#include "seal/seal.h"
#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "math_utils/matrix.h"

#include "math_utils/matrix_operations.hpp"
#include "math_utils/query_expander.hpp"
#include "marshal/marshal.hpp"
#include "concurrency/concurrency.h"
#include "utils.hpp"
#include "client_context.hpp"

#include "manager_workstream.hpp"


namespace services {

    struct ResultMatPart {
        seal::Ciphertext ctx;
        std::uint64_t row;
        std::uint64_t col;
    };

    struct WorkerInfo {
        std::uint64_t worker_number;
        std::uint64_t query_range_start;
        std::uint64_t query_range_end;
        std::vector<std::uint64_t> db_rows;
    };

    struct EpochData {
        std::map<std::string, WorkerInfo> worker_to_responsibilities;
        // following the same key as the client's db.
        std::map<std::uint64_t, std::shared_ptr<concurrency::promise<std::vector<seal::Ciphertext>>>> queries;
    };
    /**
 * WorkDistributionLedger keeps track on a distributed task.
 * it should keep hold on a working task.
 */
    struct WorkDistributionLedger {
        std::shared_mutex mtx;
        // states the workers that have already contributed their share of the work.
        std::set<std::string> contributed;

        // an ordered list of all workers this ledger should wait to.
        std::vector<std::string> worker_list;

        // result_mat is the work result of all workers.
        math_utils::matrix<seal::Ciphertext> result_mat;

        // open completion will be closed to indicate to anyone waiting.
        concurrency::Channel<int> done;
    };


    class Manager : distribicom::Manager::WithCallbackMethod_RegisterAsWorker<distribicom::Manager::Service> {
    private:
        distribicom::AppConfigs app_configs;

        std::shared_mutex mtx;

        concurrency::Counter worker_counter;
        std::map<std::pair<int, int>, std::shared_ptr<WorkDistributionLedger>> ledgers;

        std::shared_ptr<marshal::Marshaller> marshal;

#ifdef DISTRIBICOM_DEBUG
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::shared_ptr<math_utils::QueryExpander> expander;
#endif

        std::map<std::string, WorkStream *> work_streams;
        EpochData epoch_data;
    public:
        explicit Manager() {};

        explicit Manager(const distribicom::AppConfigs &app_configs) :
                app_configs(app_configs),

                marshal(marshal::Marshaller::Create(utils::setup_enc_params(app_configs)))
#ifdef DISTRIBICOM_DEBUG
                , matops(math_utils::MatrixOperations::Create(

                math_utils::EvaluatorWrapper::Create(utils::setup_enc_params(app_configs)))),
                expander(math_utils::QueryExpander::Create(utils::setup_enc_params(app_configs)))
#endif
        {};


        // a worker should send its work, along with credentials of what it sent.
        ::grpc::Status
        ReturnLocalWork(::grpc::ServerContext *context, ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                        ::distribicom::Ack *response) override;

        void verify_worker(const std::vector<ResultMatPart>& parts, std::string worker_creds){

        };

        void put_in_result_matrix(const std::vector<ResultMatPart>& parts, ClientDB& all_clients){
            all_clients.mutex.lock_shared();
            for(const auto & partial_answer : parts){
                math_utils::EmbeddedCiphertext ptx_embedding;
                this->matops->w_evaluator->get_ptx_embedding(partial_answer.ctx, ptx_embedding);

                for(size_t i = 0;i<ptx_embedding.size();i++){
                    auto current_location = all_clients.id_to_info[partial_answer.col]->partial_answer->pos(partial_answer.row ,i);
                    all_clients.id_to_info[partial_answer.col]->partial_answer->data[current_location] = ptx_embedding[i];
                }
                all_clients.id_to_info[partial_answer.col]->answer_count+=1;
            }
            all_clients.mutex.unlock_shared();
        };

        void calculate_final_answer(ClientDB& all_clients){
            for(const auto & client : all_clients.id_to_info){
                auto result = matops->async_mat_mult(this->epoch_data.queries[client.first] ,client.second->partial_answer);

            }
        };


        // todo: break up query distribution, create unified structure for id lookups, modify ledger accoringly

        std::shared_ptr<WorkDistributionLedger> distribute_work(
                const math_utils::matrix<seal::Plaintext> &db,
                const ClientDB &all_clients,
                int rnd,
                int epoch
#ifdef DISTRIBICOM_DEBUG
                , const seal::GaloisKeys &expansion_key

#endif
        );

        void wait_for_workers(int i);


        void create_res_matrix(const math_utils::matrix<seal::Plaintext> &db,
                               const ClientDB &all_clients,
                               const seal::GaloisKeys &expansion_key,
                               std::shared_ptr<WorkDistributionLedger> &ledger) const;

        /**
         *  assumes num workers map well to db and queries
         */
        map<string, WorkerInfo> map_workers_to_responsibilities(uint64_t num_queries);

        void send_galois_keys(const ClientDB &all_clients);

        void send_db(const math_utils::matrix<seal::Plaintext> &db, int rnd, int epoch);

        void send_queries(const ClientDB &all_clients);

        ::grpc::ServerWriteReactor<::distribicom::WorkerTaskPart> *RegisterAsWorker(
                ::grpc::CallbackServerContext *ctx/*context*/,
                const ::distribicom::WorkerRegistryRequest *rqst/*request*/) override;

        void close() {
            mtx.lock();
            for (auto ptr: work_streams) {
                ptr.second->close();
            }
            mtx.unlock();
        }

        /**
         * assumes the given db is thread-safe.
         */
        void new_epoch(const ClientDB &db);
    };
}
