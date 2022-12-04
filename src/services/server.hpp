
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
        distribicom::ClientQueryRequest query_info_marshaled;
        distribicom::GaloisKeys galois_keys_marshaled;

        PirQuery query;
        seal::GaloisKeys galois_keys;
        std::unique_ptr<distribicom::Client::Stub> client_stub;
    };

    struct ClientDB {
        std::shared_mutex mutex;
        std::map<std::uint32_t, std::unique_ptr<ClientInfo>> id_to_info;
        std::uint64_t client_counter=0;
    };


    // uses both the Manager and the Server services to complete a full distribicom server.
    class FullServer final : public distribicom::Server::Service{
        // used for tests
        services::DB<seal::Plaintext> db;
        services::DB<seal::Ciphertext> queries;
        services::DB<seal::GaloisKeys> gal_keys;

        // using composition to implement the interface of the manager.
        services::Manager manager;

        distribicom::Configs pir_configs;
        PirParams pir_params;
        seal::EncryptionParameters enc_params;
        std::unique_ptr<PIRClient> client;

        // concurrency stuff
        ClientDB client_query_manager;




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

        grpc::Service *get_manager_service();

        // The server gives the manager the current state of the DB and the queries.
        // The manager then distributes the work to the workers and returns the results.
        std::shared_ptr<WorkDistributionLedger> distribute_work();

        void wait_for_workers(int i);

        void start_epoch();

    private:
        void finish_construction(const distribicom::AppConfigs &app_configs);
    };
}
