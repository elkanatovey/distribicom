
#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "manager.hpp"
#include "db.hpp"
#include "pir_client.hpp"
#include <future>

namespace services {
    /**
 * ClientInfo stores objects related to an individual client
 */
    struct ClientInfo {
        distribicom::QueryInfo client_info_marshaled;
        std::string stub_id;
        PirQuery query;
        seal::GaloisKeys galois_keys;
    };

    // uses both the Manager and the Server services to complete a full distribicom server.
    class FullServer final : public distribicom::Server::Service{
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

        // concurrency stuff
        std::map<std::string, std::unique_ptr<distribicom::Client::Stub>> client_stubs;
        std::map<std::string, std::uint32_t> id_to_mailbox;
        std::map<std::uint32_t, std::unique_ptr<ClientInfo>> query_bookeeper;
        std::mutex registration_mutex;
        std::uint64_t client_counter=0;




        std::vector<std::future<int>> db_write_requests;

    public:
        explicit FullServer( const distribicom::AppConfigs& app_configs);



        // mainly for testing.
        explicit FullServer(math_utils::matrix<seal::Plaintext> &db,
                   math_utils::matrix<seal::Ciphertext> &queries,
                   math_utils::matrix<seal::GaloisKeys> &gal_keys,
                   const distribicom::AppConfigs &app_configs);;

        grpc::Status
        RegisterAsClient(grpc::ServerContext *context, const distribicom::ClientRegistryRequest *request,
                     distribicom::ClientRegistryReply *response) override;

        grpc::Status
        StoreQuery(grpc::ServerContext *context, const distribicom::ClientQueryRequest *request,
                         distribicom::Ack *response) override;

        grpc::Status
        WriteToDB(grpc::ServerContext *context, const distribicom::WriteRequest *request,
                   distribicom::Ack *response) override;

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

    private:
        void finish_construction(const distribicom::AppConfigs &app_configs);;
    };
}
