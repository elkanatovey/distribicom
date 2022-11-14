#include "client_service.hpp"

namespace services {
    ClientListener::ClientListener(distribicom::AppConfigs &app_configs) {

    }

    grpc::Status ClientListener::Answer(grpc::ServerContext *context, const distribicom::Ciphertext *answer,
                                        distribicom::Ack *response) {
        return Service::Answer(context, answer, response);
    }

    grpc::Status
    ClientListener::TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest *request,
                                 distribicom::Ack *response) {
        return Service::TellNewRound(context, request, response);
    }
} // services