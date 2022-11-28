#include "client_service.hpp"
#include "utils.hpp"

namespace services {
    ClientListener::ClientListener(distribicom::ClientConfigs &&_client_configs): client_configs(std::move(_client_configs)){
        enc_params = utils::setup_enc_params(client_configs.app_configs());
        seal::Blake2xbPRNGFactory factory;
        this->answering_machine = factory.create({}); // @todo maybe receive as param for testing

        auto app_configs = client_configs.app_configs();
        const auto& configs = app_configs.configs();
        gen_pir_params(configs.number_of_elements(), configs.size_per_element(),
                       configs.dimensions(), enc_params, pir_params,  configs.use_symmetric(),
                       configs.use_batching(), configs.use_recursive_mod_switching());
        client = make_unique<PIRClient>(PIRClient(enc_params, pir_params));

        mrshl = marshal::Marshaller::Create(enc_params);

        server_conn = std::make_unique<distribicom::Server::Stub>(distribicom::Server::Stub(
                grpc::CreateChannel(
                        app_configs.main_server_hostname(),
                        grpc::InsecureChannelCredentials()
                )
        ));
        std::string galois_key = mrshl->marshal_seal_object(client->generate_galois_keys());

        grpc::ClientContext context;
        distribicom::ClientRegistryRequest request;

        request.set_client_port(client_configs.client_port());
        request.set_galois_keys(galois_key);

        auto status = server_conn->RegisterAsClient(&context, request, &mail_data);
        if(!status.ok()){
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }

    }

    grpc::Status ClientListener::Answer(grpc::ServerContext *context, const distribicom::PirResponse *answer,
                                        distribicom::Ack *response) {
        auto ans =  mrshl->unmarshal_pir_response(*answer);
        current_answer = client->decode_reply(ans);
        response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status
    ClientListener::TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest *request,
                                 distribicom::WriteRequest *response) {
        round = request->round();
        auto msg = answering_machine->generate() % 256;
        response->set_data(std::to_string(msg));
        response->set_ptxnumber(client->get_fv_index(this->mail_data.mailbox_id())); // index of FV plaintext
        response->set_whereinptx(client->get_fv_offset(this->mail_data.mailbox_id())); // offset in FV plaintext
//        response->set_success(true);
        // @todo maybe make msgs bigger
        return grpc::Status::OK;
    }

    void ClientListener::Query(std::uint64_t desiredIndex) {
        auto query = client->generate_query(desiredIndex);
        distribicom::ClientQueryRequest request;
        mrshl->marshal_query_vector(query, request);
        grpc::ClientContext context;
        distribicom::Ack ack;

        auto status = server_conn->StoreQuery(&context, request, &ack);
        if(!status.ok()){
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void ClientListener::Query() {
        Query(client->get_fv_index(mail_data.mailbox_id()));
    }


} // services