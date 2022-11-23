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

        PirParams pir_params;
        seal::EncryptionParameters enc_params;

    public:
        explicit ClientListener(distribicom::ClientConfigs &_client_configs);

        grpc::Status
        Answer(grpc::ServerContext *context, const distribicom::Ciphertext *answer,
               distribicom::Ack *response)override;

        grpc::Status
        TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest* request, distribicom::Ack* response)override;


        grpc::Status Query(std::uint64_t desiredIndex);

        /**
         *
         * @return
         */
        grpc::Status Query();
    };

} // services
