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
    return Service::RegisterAsClient(context, request, response);
}

grpc::Status
services::FullServer::StoreQuery(grpc::ServerContext *context, const distribicom::ClientQueryRequest *request,
                                 distribicom::Ack *response) {
    return Service::StoreQuery(context, request, response);
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
