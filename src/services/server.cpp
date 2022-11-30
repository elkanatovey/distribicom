//
// Created by Jonathan Weiss on 24/10/2022.
//

#include "server.hpp"


services::FullServer::FullServer(math_utils::matrix<seal::Plaintext> &db, math_utils::matrix<seal::Ciphertext> &queries,
                                 math_utils::matrix<seal::GaloisKeys> &gal_keys,
                                 const distribicom::AppConfigs &app_configs) :
        db(db), queries(queries), gal_keys(gal_keys), manager(app_configs) {
    finish_construction(app_configs);

}

services::FullServer::FullServer(const distribicom::AppConfigs &app_configs) :
        db(0, 0), queries(0, 0), gal_keys(0, 0), manager(app_configs)  {
    finish_construction(app_configs);

}

void services::FullServer::finish_construction(const distribicom::AppConfigs &app_configs) {
    pir_configs = app_configs.configs();
    enc_params = utils::setup_enc_params(app_configs);
    const auto& configs = app_configs.configs();
    gen_pir_params(configs.number_of_elements(), configs.size_per_element(),
                   configs.dimensions(), enc_params, pir_params, configs.use_symmetric(),
                   configs.use_batching(), configs.use_recursive_mod_switching());
    client = make_unique<PIRClient>(PIRClient(enc_params, pir_params));
}

grpc::Status
services::FullServer::RegisterAsClient(grpc::ServerContext *context, const distribicom::ClientRegistryRequest *request,
                                       distribicom::ClientRegistryReply *response) {


    try {

        auto requesting_client = utils::extract_ipv4(context);
        std::string subscribing_client_address = context->auth_context()->GetPeerIdentityPropertyName(); //@todo see if valid

        // creating stub to the client:
        auto client_conn = std::make_unique<distribicom::Client::Stub>(distribicom::Client::Stub(
                grpc::CreateChannel(
                        subscribing_client_address,
                        grpc::InsecureChannelCredentials()
                )
        ));
        auto client_info = std::make_unique<services::ClientInfo>(ClientInfo());
        client_info->client_info_marshaled.set_galois_keys(request->galois_keys());

        registration_mutex.lock();
        if (client_stubs.find(requesting_client) == client_stubs.end()) {
            client_info->client_info_marshaled.set_client_mailbox(client_counter);
            id_to_mailbox.insert({requesting_client ,client_counter});
            query_bookeeper.insert({client_counter, std::move(client_info)});

            client_stubs.insert({requesting_client, std::move(client_conn)});
            response->set_mailbox_id(client_counter);
            client_counter+=1;


        }
        registration_mutex.unlock();


    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        return {grpc::StatusCode::INTERNAL, e.what()};
    }

    response->set_num_mailboxes(pir_configs.number_of_elements());

    return grpc::Status::OK;
}

//@todo this assumes that no one is registering, very dangerous
grpc::Status
services::FullServer::StoreQuery(grpc::ServerContext *context, const distribicom::ClientQueryRequest *request,
                                 distribicom::Ack *response) {

    auto id = context->auth_context()->GetPeerIdentityPropertyName();
    auto num_id = id_to_mailbox[id];
    query_bookeeper[num_id]->client_info_marshaled.CopyFrom(*request);
    response->set_success(true);
    return grpc::Status::OK;
}

//@todo fix this to translate bytes to coeffs properly when writing
grpc::Status services::FullServer::WriteToDB(grpc::ServerContext *context, const distribicom::WriteRequest *request,
                                             distribicom::Ack *response) {
    this->db.write();
    db_write_requests.push_back( std::async(
            [&]( distribicom::WriteRequest request) {
                std::uint8_t msg = std::stoi(request.data());
                std::vector<uint64_t> msg1 = {msg};
                db.write(msg1, request.ptxnumber(), request.whereinptx(), this->client.get());
                return 0;
            },
            *request
    ));

    return grpc::Status::OK;
}
