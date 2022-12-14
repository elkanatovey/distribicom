
#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "manager.hpp"
#include "db.hpp"
#include "pir_client.hpp"
#include "client_context.hpp"
#include <future>

namespace services {


    // uses both the Manager and the Server services to complete a full distribicom server.
    class FullServer final : public distribicom::Server::Service {

        // using composition to implement the interface of the manager.
        services::Manager manager;

        distribicom::Configs pir_configs;
        PirParams pir_params;
        seal::EncryptionParameters enc_params;


        std::vector<std::future<int>> db_write_requests;

    public:
        // mainly for testing.
        explicit FullServer(math_utils::matrix<seal::Plaintext> &db,
                            std::map<uint32_t, std::unique_ptr<services::ClientInfo>> &client_db,
                            const distribicom::AppConfigs &app_configs);

//        explicit FullServer(const distribicom::AppConfigs &app_configs);


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

        void publish_galois_keys();

        void tell_new_round();

        void publish_answers();

        void send_stop_signal();

        void learn_about_rouge_workers(std::shared_ptr<WorkDistributionLedger>);

        void run_step_2(std::shared_ptr<WorkDistributionLedger>);

        void close() { manager.close(); }

        // for testing:
        const ClientDB &get_client_db() { return manager.client_query_manager; }

    private:
        void init_pir_data(const distribicom::AppConfigs &app_configs);
    };
}
