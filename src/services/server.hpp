
#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "manager.hpp"
#include "db.hpp"
#include "pir_client.hpp"

namespace services {
    // uses both the Manager and the Server services to complete a full distribicom server.
    class FullServer {
        // used for tests
        pir_primitives::DB<seal::Plaintext> db;
        pir_primitives::DB<seal::Ciphertext> queries;
        pir_primitives::DB<seal::GaloisKeys> gal_keys;

        // using composition to implement the interface of the manager.
        services::Manager manager;

        distribicom::Configs pir_configs;
        PirParams pir_params;
        seal::EncryptionParameters enc_params;
        std::unique_ptr<PIRClient> client;

    public:
        explicit FullServer( const distribicom::AppConfigs& app_configs) :
                db(0, 0), queries(0, 0), gal_keys(0, 0), manager(app_configs)  {
            finish_construction(app_configs);

        }

        void finish_construction(const distribicom::AppConfigs &app_configs) {
            pir_configs = app_configs.configs();
            enc_params = utils::setup_enc_params(app_configs);
            const auto& configs = app_configs.configs();
            gen_pir_params(configs.number_of_elements(), configs.size_per_element(),
                           configs.dimensions(), enc_params, pir_params, configs.use_symmetric(),
                           configs.use_batching(), configs.use_recursive_mod_switching());
            client = make_unique<PIRClient>(PIRClient(enc_params, pir_params));
        };

        // mainly for testing.
        FullServer(math_utils::matrix<seal::Plaintext> &db,
                   math_utils::matrix<seal::Ciphertext> &queries,
                   math_utils::matrix<seal::GaloisKeys> &gal_keys,
                   const distribicom::AppConfigs &app_configs) :
                db(db), queries(queries), gal_keys(gal_keys), manager(app_configs) {
            finish_construction(app_configs);

        };

        grpc::Service *get_manager_service() {
            return (grpc::Service *) (&manager);
        };

        // The server gives the manager the current state of the DB and the queries.
        // The manager then distributes the work to the workers and returns the results.
        std::unique_ptr<WorkDistributionLedger> distribute_work() {
            std::unique_ptr<services::WorkDistributionLedger> ledger;

            // block is to destroy the db and querries handles.
            {
                auto db_handle = db.many_reads();
                auto queries_handle = queries.many_reads();
                // todo: set specific round and handle.
                ledger = manager.distribute_work(db_handle.mat, queries_handle.mat, 1, 1);
            }
            // todo, should behave as a promise.
            ledger->done.read_for(std::chrono::milliseconds(1000)); // todo: set specific timeout..
            // i don't want to wait on a timeout!
            return ledger;
        }

        void wait_for_workers(int i) {
            manager.wait_for_workers(i);
        }

        void start_epoch() {
            //todo: start epoch for registered clients as well -> make them send queries.
            auto handle = gal_keys.many_reads();
            manager.send_galois_keys(handle.mat);
//            wait_for_workers(0); todo: wait for app_configs.num_workers
        }
    };
}
