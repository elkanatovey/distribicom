#pragma once

#include "distribicom.grpc.pb.h"
#include "marshal.hpp"

namespace services {

    // listener run by query owner to receive answer
    class ClientListener final : public distribicom::Client::Service {

        distribicom::AppConfigs app_configs;
        std::shared_ptr<marshal::Marshaller> mrshl;


        explicit ClientListener(distribicom::AppConfigs &app_configs);

        grpc::Status
        Answer(grpc::ServerContext *context, const distribicom::Ciphertext *answer,
               distribicom::Ack *response)override;

        grpc::Status
        TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest* request, distribicom::Ack* response)override;

    };

} // services
