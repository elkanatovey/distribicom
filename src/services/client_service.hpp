#pragma once

#include "distribicom.grpc.pb.h"
#include "marshal/marshal.hpp"
#include "pir_client.hpp"

namespace services {

    // listener run by query owner to receive answer
    class ClientListener final : public distribicom::Client::Service {

        distribicom::ClientConfigs client_configs;
        std::shared_ptr<marshal::Marshaller> mrshl;
        std::unique_ptr<distribicom::Server::Stub> server_conn;
        std::unique_ptr<PIRClient> client;
        distribicom::ClientRegistryReply mail_data;

        std::shared_ptr<seal::UniformRandomGenerator> answering_machine; // for testing

        PirParams pir_params;
        seal::EncryptionParameters enc_params;
        seal::Plaintext current_answer;
        std::uint32_t round;

    public:
        explicit ClientListener(distribicom::ClientConfigs &_client_configs);

        /**
         *  called by main server when answer to pir query is ready
         */
        grpc::Status
        Answer(grpc::ServerContext *context, const distribicom::PirResponse *answer,
               distribicom::Ack *response)override;

        /**
         * called by main server to inform when new round starts, returns new item to write to db
         */
        grpc::Status
        TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest* request, distribicom::WriteRequest* response)override;


        void Query(std::uint64_t desiredIndex);

        /**
         *
         * @return
         */
        void Query();

//        string
    };

} // services
