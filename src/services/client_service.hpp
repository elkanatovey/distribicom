#pragma once

#include "distribicom.grpc.pb.h"
#include "marshal.hpp"
#include "pir_client.hpp"

namespace services {

    // listener run by query owner to receive answer
    class ClientListener final : public distribicom::Client::Service {

        distribicom::AppConfigs app_configs;
        std::shared_ptr<marshal::Marshaller> mrshl;
        std::unique_ptr<distribicom::Server::Stub> server_conn;
        std::unique_ptr<PIRClient> client;

    public:
        explicit ClientListener(distribicom::AppConfigs &_app_configs);

        grpc::Status
        Answer(grpc::ServerContext *context, const distribicom::Ciphertext *answer,
               distribicom::Ack *response)override;

        grpc::Status
        TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest* request, distribicom::Ack* response)override;

        seal::EncryptionParameters setup_enc_params() const;
    };

} // services
