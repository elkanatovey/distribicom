//
// Created by Jonathan Weiss on 24/10/2022.
//

#include "server.hpp"
#include "client_context.hpp"


services::FullServer::FullServer(math_utils::matrix<seal::Plaintext> &db, std::map<uint32_t,
        std::unique_ptr<services::ClientInfo>> &client_db,
                                 const distribicom::AppConfigs &app_configs) :
        db(db), manager(app_configs), pir_configs(app_configs.configs()),
        enc_params(utils::setup_enc_params(app_configs)) {
    this->client_query_manager.client_counter = client_db.size();
    this->client_query_manager.id_to_info = std::move(client_db);
    init_pir_data(app_configs);

}

services::FullServer::FullServer(const distribicom::AppConfigs &app_configs) :
        db(app_configs.configs().db_rows(), app_configs.configs().db_cols()), manager(app_configs),
        pir_configs(app_configs.configs()), enc_params(utils::setup_enc_params(app_configs)) {
    init_pir_data(app_configs);

}

void services::FullServer::init_pir_data(const distribicom::AppConfigs &app_configs) {
    const auto &configs = app_configs.configs();
    gen_pir_params(configs.number_of_elements(), configs.size_per_element(),
                   configs.dimensions(), enc_params, pir_params, configs.use_symmetric(),
                   configs.use_batching(), configs.use_recursive_mod_switching());
}

grpc::Status
services::FullServer::RegisterAsClient(grpc::ServerContext *context, const distribicom::ClientRegistryRequest *request,
                                       distribicom::ClientRegistryReply *response) {


    try {

        auto requesting_client = utils::extract_ip(context);
        std::string subscribing_client_address = requesting_client + ":" + std::to_string(request->client_port());

        // creating stub to the client:
        auto client_conn = std::make_unique<distribicom::Client::Stub>(distribicom::Client::Stub(
                grpc::CreateChannel(
                        subscribing_client_address,
                        grpc::InsecureChannelCredentials()
                )
        ));

        auto client_info = std::make_unique<ClientInfo>(ClientInfo());
        client_info->galois_keys_marshaled.set_keys(request->galois_keys());
        client_info->client_stub = std::move(client_conn);

        std::unique_lock lock(client_query_manager.mutex);
        client_info->galois_keys_marshaled.set_key_pos(client_query_manager.client_counter);
        client_query_manager.id_to_info.insert(
                {client_query_manager.client_counter, std::move(client_info)});
        response->set_mailbox_id(client_query_manager.client_counter);
        client_query_manager.client_counter += 1;

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

    auto id = request->mailbox_id();

    std::unique_lock lock(client_query_manager.mutex);
    if (client_query_manager.id_to_info.find(id) == client_query_manager.id_to_info.end()) {
        return {grpc::StatusCode::NOT_FOUND, "Client not found"};
    }

    client_query_manager.id_to_info[id]->query_info_marshaled.CopyFrom(*request);
    response->set_success(true);
    return grpc::Status::OK;
}

//@todo fix this to translate bytes to coeffs properly when writing
grpc::Status services::FullServer::WriteToDB(grpc::ServerContext *context, const distribicom::WriteRequest *request,
                                             distribicom::Ack *response) {
//    this->db.write();
//    db_write_requests.push_back(std::async(
//            [&](distribicom::WriteRequest request) {
//                std::uint8_t msg = std::stoi(request.data());
//                std::vector<uint64_t> msg1 = {msg};
//                db.write(msg1, request.ptxnumber(), request.whereinptx(), this->client.get());
//                return 0;
//            },
//            *request
//    ));

    return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

std::shared_ptr<services::WorkDistributionLedger> services::FullServer::distribute_work() {
    std::shared_ptr<services::WorkDistributionLedger> ledger;

    // block is to destroy the db handle.
    {
        auto db_handle = db.many_reads();

        std::shared_lock client_db_lock(client_query_manager.mutex);

        ledger = manager.distribute_work(db_handle.mat, client_query_manager, 1, 1
#ifdef DISTRIBICOM_DEBUG
                                         ,client_query_manager.id_to_info.begin()->second->galois_keys
#endif
        );
    }

    return ledger;
}

void services::FullServer::start_epoch() {
    std::shared_lock client_db_lock(client_query_manager.mutex);

    manager.new_epoch(client_query_manager);
    manager.send_galois_keys(client_query_manager);
}

void services::FullServer::wait_for_workers(int i) {
    manager.wait_for_workers(i);
}

grpc::Service *services::FullServer::get_manager_service() {
    return (grpc::Service *) (&manager);
}

void services::FullServer::publish_galois_keys() {
    throw std::logic_error("not implemented");
}

void services::FullServer::publish_answers() {
    throw std::logic_error("not implemented");
}

void services::FullServer::send_stop_signal() {
    throw std::logic_error("not implemented");
}

void services::FullServer::learn_about_rouge_workers(std::shared_ptr<WorkDistributionLedger>) {
    throw std::logic_error("not implemented");
}

void services::FullServer::run_step_2(std::shared_ptr<WorkDistributionLedger>) {
    throw std::logic_error("not implemented");
}

void services::FullServer::tell_new_round() {
    throw std::logic_error("not implemented");
}
