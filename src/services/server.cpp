//
// Created by Jonathan Weiss on 24/10/2022.
//

#include "server.hpp"
#include "client_context.hpp"


services::FullServer::FullServer(math_utils::matrix<seal::Plaintext> &db, math_utils::matrix<seal::Ciphertext> &queries,
                                 math_utils::matrix<seal::GaloisKeys> &gal_keys,
                                 const distribicom::AppConfigs &app_configs) :
        db(db), queries(queries), gal_keys(gal_keys), manager(app_configs) {
    finish_construction(app_configs);

}

services::FullServer::FullServer(const distribicom::AppConfigs &app_configs) :
        db(0, 0), queries(0, 0), gal_keys(0, 0), manager(app_configs) {
    finish_construction(app_configs);

}

void services::FullServer::finish_construction(const distribicom::AppConfigs &app_configs) {
    pir_configs = app_configs.configs();
    enc_params = utils::setup_enc_params(app_configs);
    const auto &configs = app_configs.configs();
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

    // block is to destroy the db and querries handles.
    {
        auto db_handle = db.many_reads();
        auto queries_handle = queries.many_reads();
        // todo: set specific round and handle.


        ledger = manager.distribute_work(db_handle.mat, queries_handle.mat, 1, 1,
#ifdef DISTRIBICOM_DEBUG
                                         gal_keys.many_reads().mat.data[0]
#endif
        );
    }
    // todo, should behave as a promise.
    ledger->done.read_for(std::chrono::milliseconds(1000)); // todo: set specific timeout..
    // i don't want to wait on a timeout!
    return ledger;
}

void services::FullServer::start_epoch() {
    client_query_manager.mutex.lock_shared();
    manager.map_workers_to_responsibilities(client_query_manager.client_counter, pir_configs.db_rows());
    client_query_manager.mutex.unlock_shared();

    //todo: start epoch for registered clients as well -> make them send queries.
    auto handle = gal_keys.many_reads();
    manager.send_galois_keys(handle.mat);
//            wait_for_workers(0); todo: wait for app_configs.num_workers
}

void services::FullServer::wait_for_workers(int i) {
    manager.wait_for_workers(i);
}

grpc::Service *services::FullServer::get_manager_service() {
    return (grpc::Service * )(&manager);
}
