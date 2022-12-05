#pragma once

#include "seal/seal.h"
#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "../math_utils/matrix.h"

#include "math_utils/matrix_operations.hpp"
#include "math_utils/query_expander.hpp"
#include "marshal/marshal.hpp"
#include "concurrency/concurrency.h"
#include "utils.hpp"
#include "client_context.hpp"


namespace services {

    struct WorkerInfo {
        std::uint64_t worker_number;
        std::uint64_t query_range_start;
        std::uint64_t query_range_end;
        std::uint64_t db_row;
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

    // contains the workers and knows how to distribute their work.
    // should be able to give a Promise for a specific round and fullfill it once all workers have sent their jobs.
    // can use Frievalds to verify their work.

    class Manager : public distribicom::Manager::Service {
    private:
        distribicom::AppConfigs app_configs;

        std::mutex mtx; // todo: use shared_mtx,
        std::map<std::string, std::unique_ptr<distribicom::Worker::Stub>> worker_stubs;

        std::map<std::string, WorkerInfo> worker_name_to_work_responsible_for; // todo: refactor into struct
        concurrency::Counter worker_counter;
        std::map<std::pair<int, int>, std::shared_ptr<WorkDistributionLedger>> ledgers;

        std::shared_ptr<marshal::Marshaller> marshal;

#ifdef DISTRIBICOM_DEBUG
        std::shared_ptr<math_utils::MatrixOperations> matops;
        std::shared_ptr<math_utils::QueryExpander> expander;
#endif


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

        // PartialWorkStream i save this on my server map.
        ::grpc::Status
        RegisterAsWorker(::grpc::ServerContext *context, const ::distribicom::WorkerRegistryRequest *request,
                         ::distribicom::Ack *response) override;

        // a worker should send its work, along with credentials of what it sent.
        ::grpc::Status
        ReturnLocalWork(::grpc::ServerContext *context, ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                        ::distribicom::Ack *response) override;

        // This one is the holder of the DB.


        // todo:
//        void distribute_work(const math_utils::matrix<seal::Plaintext> &db);

        // todo: break up query distribution, create unified structure for id lookups, modify ledger accoringly

        std::shared_ptr<WorkDistributionLedger> distribute_work(
                const math_utils::matrix<seal::Plaintext> &db,
                const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                int rnd,
                int epoch,
#ifdef DISTRIBICOM_DEBUG
                const seal::GaloisKeys &expansion_key
#endif
        );

        std::shared_ptr<WorkDistributionLedger> sendtask(const math_utils::matrix<seal::Plaintext> &db,
                                                         const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                                                         grpc::ClientContext &context,
                                                         std::shared_ptr<WorkDistributionLedger> ptr);

        void wait_for_workers(int i);

        void send_galois_keys(const math_utils::matrix<seal::GaloisKeys> &matrix);


        void create_res_matrix(const math_utils::matrix<seal::Plaintext> &db,
                               const math_utils::matrix<seal::Ciphertext> &compressed_queries,
                               const seal::GaloisKeys &expansion_key,
                               std::shared_ptr<WorkDistributionLedger> &ledger) const;

        // assumes num workers map well to db and queries
        void map_workers_to_responsibilities(uint64_t num_rows, uint64_t num_queries);

        void send_galois_keys( ClientDB &all_clients);

        void send_db(const math_utils::matrix<seal::Plaintext> &db, grpc::ClientContext &context);
    };
}



//for (auto &worker: worker_stubs) {
//{ // this stream is released at the end of this scope.
//auto stream = worker.second->SendTask(&context, &response);
//auto range_start = worker_name_to_work_responsible_for[worker.first].query_range_start;
//auto  range_end = worker_name_to_work_responsible_for[worker.first].query_range_end;
//
//for (std::uint64_t i = range_start; i < range_end; ++i) {
//prt.mutable_gkey()->CopyFrom(all_clients.id_to_info[i]->galois_keys_marshaled);
//stream->Write(prt);
//}
//stream->WritesDone();
//auto status = stream->Finish();
//if (!status.ok()) {
//std::cout << "manager:: distribute_work:: transmitting galois gal_key to " << worker.first <<
//" failed: " << status.error_message() << std::endl;
//continue;
//}
//}
//}
