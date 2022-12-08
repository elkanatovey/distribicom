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

    struct WorkerInfo {
        std::uint64_t worker_number;
        std::uint64_t query_range_start;
        std::uint64_t query_range_end;
        std::vector<std::uint64_t> db_rows;
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

        std::map<std::string, WorkerInfo> worker_name_to_work_responsible_for; // todo: refactor into struct
        concurrency::Counter worker_counter;
        std::map<std::pair<int, int>, std::shared_ptr<WorkDistributionLedger>> ledgers;

        std::shared_ptr<marshal::Marshaller> marshal;

#ifdef DISTRIBICOM_DEBUG
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::shared_ptr<math_utils::QueryExpander> expander;
#endif

        std::map<std::string, WorkStream *> work_streams;

    public:
        explicit Manager() {};

        explicit Manager(const distribicom::AppConfigs &app_configs) :
                app_configs(app_configs),
                marshal(marshal::Marshaller::Create(utils::setup_enc_params(app_configs))),
#ifdef DISTRIBICOM_DEBUG
                matops(math_utils::MatrixOperations::Create(
                        math_utils::EvaluatorWrapper::Create(utils::setup_enc_params(app_configs)))),
                expander(math_utils::QueryExpander::Create(utils::setup_enc_params(app_configs)))
#endif
        {};


        // a worker should send its work, along with credentials of what it sent.
        ::grpc::Status
        ReturnLocalWork(::grpc::ServerContext *context, ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                        ::distribicom::Ack *response) override;


        // todo: break up query distribution, create unified structure for id lookups, modify ledger accoringly

        std::shared_ptr<WorkDistributionLedger> distribute_work(
                const math_utils::matrix<seal::Plaintext> &db,
                const ClientDB &all_clients,
                int rnd,
                int epoch,
#ifdef DISTRIBICOM_DEBUG
                const seal::GaloisKeys &expansion_key
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
        void map_workers_to_responsibilities(uint64_t num_rows, uint64_t num_queries);

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
    };
}
