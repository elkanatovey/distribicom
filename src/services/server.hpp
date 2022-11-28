
#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "manager.hpp"
#include "db.hpp"

namespace services {
    // uses both the Manager and the Server services to complete a full distribicom server.
    class FullServer {
        // used for tests
        DB<seal::Plaintext> db;
        DB<seal::Ciphertext> queries;
        DB<seal::GaloisKeys> gal_keys;

        // using composition to implement the interface of the manager.
        services::Manager manager;


    public:
        explicit FullServer(const distribicom::AppConfigs &app_configs) :
                db(0, 0), queries(0, 0), gal_keys(0, 0), manager(app_configs) {};

        // mainly for testing.
        FullServer(math_utils::matrix<seal::Plaintext> &db,
                   math_utils::matrix<seal::Ciphertext> &queries,
                   math_utils::matrix<seal::GaloisKeys> &gal_keys,
                   const distribicom::AppConfigs &app_configs) :
                db(db), queries(queries), gal_keys(gal_keys), manager(app_configs) {};

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
    };
}
