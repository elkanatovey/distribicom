//
// Created by Jonathan Weiss on 24/10/2022.
//

#include "server.hpp"
#include "client_context.hpp"


services::FullServer::FullServer(math_utils::matrix<seal::Plaintext> &db, std::map<uint32_t,
        std::unique_ptr<services::ClientInfo>> &client_db,
                                 const distribicom::AppConfigs &app_configs) :
        db(db), manager(app_configs, client_db), pir_configs(app_configs.configs()),
        enc_params(utils::setup_enc_params(app_configs)) {
    init_pir_data(app_configs);

}

//services::FullServer::FullServer(const distribicom::AppConfigs &app_configs) :
//        db(app_configs.configs().db_rows(), app_configs.configs().db_cols()), manager(app_configs),
//        pir_configs(app_configs.configs()), enc_params(utils::setup_enc_params(app_configs)) {
//    init_pir_data(app_configs);
//
//}

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
        response->set_mailbox_id(manager.client_query_manager.add_client(context, request, pir_configs, pir_params));
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

    std::unique_lock lock(*manager.client_query_manager.mutex);
    if (manager.client_query_manager.id_to_info.find(id) == manager.client_query_manager.id_to_info.end()) {
        return {grpc::StatusCode::NOT_FOUND, "Client not found"};
    }

    manager.client_query_manager.id_to_info[id]->query_info_marshaled.CopyFrom(*request);
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

//        // todo: set specific round and handle.

        ledger = manager.distribute_work(db_handle.mat, manager.client_query_manager, 1, 1
#ifdef DISTRIBICOM_DEBUG
                                         , manager.client_query_manager.id_to_info.begin()->second->galois_keys
#endif
        );
    }

    return ledger;
}

void services::FullServer::start_epoch() {
    std::shared_lock client_db_lock(*manager.client_query_manager.mutex);

    manager.new_epoch(manager.client_query_manager);
    manager.send_galois_keys(manager.client_query_manager);
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
