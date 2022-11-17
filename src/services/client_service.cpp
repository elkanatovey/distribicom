#include "client_service.hpp"
#include "utils.hpp"

namespace services {
    ClientListener::ClientListener(distribicom::AppConfigs &_app_configs): app_configs(_app_configs), mrshl(),
    enc_params(utils::setup_enc_params(_app_configs)){

        gen_pir_params(app_configs.configs().number_of_elements(), app_configs.configs().size_per_element(),
                       app_configs.configs().dimensions(), enc_params, pir_params,  app_configs.configs().use_symmetric(),
                       app_configs.configs().use_batching(), app_configs.configs().use_recursive_mod_switching());
        client = make_unique<PIRClient>(PIRClient(enc_params, pir_params));

        mrshl = marshal::Marshaller::Create(enc_params);

        manager_conn = std::make_unique<distribicom::Server::Stub>(distribicom::Server::Stub(
                grpc::CreateChannel(
                        app_configs().main_server_hostname(),
                        grpc::InsecureChannelCredentials()
                )
        ));

        grpc::ClientContext context;
        distribicom::Ack response;
        distribicom::WorkerRegistryRequest request;

        request.set_workerport(cnfgs.workerport());

        manager_conn->RegisterAsWorker(&context, request, &response);

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