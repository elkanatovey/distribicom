#include "client_service.hpp"
#include "utils.hpp"

namespace services {
    ClientListener::ClientListener(distribicom::ClientConfigs &_client_configs): client_configs(_client_configs), mrshl(),
    enc_params(utils::setup_enc_params(_client_configs.app_configs())){
        auto app_configs = client_configs.app_configs();
        gen_pir_params(app_configs.configs().number_of_elements(), app_configs.configs().size_per_element(),
                       app_configs.configs().dimensions(), enc_params, pir_params,  app_configs.configs().use_symmetric(),
                       app_configs.configs().use_batching(), app_configs.configs().use_recursive_mod_switching());
        client = make_unique<PIRClient>(PIRClient(enc_params, pir_params));

        mrshl = marshal::Marshaller::Create(enc_params);

        server_conn = std::make_unique<distribicom::Server::Stub>(distribicom::Server::Stub(
                grpc::CreateChannel(
                        app_configs.main_server_hostname(),
                        grpc::InsecureChannelCredentials()
                )
        ));

        grpc::ClientContext context;
        distribicom::ClientRegistryRequest request;

        request.set_client_port(client_configs.client_port());


        server_conn->RegisterAsClient(&context, request, &mail_data);

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