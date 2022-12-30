#include <iostream>
#include "services/factory.hpp"
#include "marshal/local_storage.hpp"
#include "server.hpp"
#include <chrono>

std::string get_listening_address(const distribicom::AppConfigs &cnfgs);

shared_ptr<services::FullServer>
full_server_instance(const distribicom::AppConfigs &configs, const seal::EncryptionParameters &enc_params,
                     PirParams &pir_params, std::vector<PIRClient> &clients);

std::vector<PIRClient>
create_clients(std::uint64_t size, const distribicom::AppConfigs &app_configs,
               const seal::EncryptionParameters &enc_params,
               PirParams &pir_params);

std::thread run_server(const latch &, shared_ptr<services::FullServer>, distribicom::AppConfigs &);

void verify_results(shared_ptr<services::FullServer> &sharedPtr, vector<PIRClient> &vector1);


bool is_valid_command_line_args(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <config_file>" << " <num_queries>" << std::endl;
        return false;
    }

    if (!std::filesystem::exists(argv[1])) {
        std::cout << "Config file " << argv[1] << " does not exist" << std::endl;
        return false;
    }

    if (std::stoi(argv[2]) < 1) {
        std::cout << "num queries " << argv[2] << " is invalid" << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (!is_valid_command_line_args(argc, argv)) {
        return -1;
    }

    distribicom::AppConfigs cnfgs;
    cnfgs.ParseFromString(load_from_file(argv[1]));

    std::cout << "server set up with the following configs:" << std::endl;
    std::cout << cnfgs.DebugString() << std::endl;

    std::uint64_t num_clients = std::stoi(argv[2]);
    std::cout << "server set up with " << num_clients << " queries" << std::endl;

    auto enc_params = services::utils::setup_enc_params(cnfgs);
    PirParams pir_params;
    auto clients = create_clients(num_clients, cnfgs, enc_params, pir_params);

    auto server = full_server_instance(cnfgs, enc_params, pir_params, clients);

    std::cout << "setting server" << std::endl;
    std::latch wg(1);
    std::vector<std::thread> threads;
    threads.emplace_back(run_server(wg, server, cnfgs));

    server->wait_for_workers(int(cnfgs.number_of_workers()));
    // 1 epoch
    for (int i = 0; i < 1; ++i) {
        std::cout << "setting up epoch" << std::endl;

        server->start_epoch();

        // 5 rounds in an epoch.
        for (int j = 0; j < 10; ++j) {
            // todo: start timing a full round here.
            std::cout << "distributing work" << std::endl;

            auto time_round_s = std::chrono::high_resolution_clock::now();

            auto ledger = server->distribute_work();

            ledger->done.read_for(std::chrono::milliseconds(5000)); // todo: how much time should we wait?

            // server should now inspect missing items and run the calculations for them.
            // server should also be notified by ledger about the rouge workers.
            server->learn_about_rouge_workers(ledger);

            std::cout << "done receiving" << std::endl;
            // perform step 2.
            server->run_step_2(ledger);

            // verify results.
#ifdef DISTRIBICOM_DEBUG
            verify_results(server, clients);
#endif
//            server->publish_answers();

            auto time_round_e = std::chrono::high_resolution_clock::now();
            auto time_round_us = duration_cast<std::chrono::microseconds>(time_round_e - time_round_s).count();
            std::cout << "Main_Server: round " << j << " running time: " << time_round_us / 1000 << " ms" << std::endl;
        }
    }

    server->send_stop_signal();
    wg.count_down();
    for (auto &t: threads) {
        t.join();
    }
    std::cout << "done." << std::endl;
}

void verify_results(shared_ptr<services::FullServer> &server, vector<PIRClient> &clients) {
    const auto &db = server->get_client_db();
    for (auto &[id, info]: db.id_to_info) {
        auto ptx = clients[id].decode_reply(info->final_answer->data);
        std::cout << "result for client " << id << ": " << ptx.to_string() << std::endl;
    }
}

std::thread
run_server(const latch &main_thread_done, shared_ptr<services::FullServer> server_ptr,
           distribicom::AppConfigs &configs) {

    auto t = std::thread([&] {

        grpc::ServerBuilder builder;
        builder.SetMaxMessageSize(services::constants::max_message_size);

        std::string address = get_listening_address(configs);
        builder.AddListeningPort(address, grpc::InsecureServerCredentials());

        builder.RegisterService(server_ptr->get_manager_service());

        auto server(builder.BuildAndStart());
        std::cout << "server services are online on address: " << address << std::endl;

        main_thread_done.wait();

        server->Shutdown();
        std::cout << "server services are offline." << std::endl;
    });

    // ensuring the server is up before returning.
    sleep(2);
    return t;
}

std::string get_listening_address(const distribicom::AppConfigs &cnfgs) {
    const std::string &address(cnfgs.main_server_hostname());
    return "0.0.0.0" + address.substr(address.find(':'), address.size());
}


std::vector<PIRClient> create_clients(std::uint64_t size, const distribicom::AppConfigs &app_configs,
                                      const seal::EncryptionParameters &enc_params,
                                      PirParams &pir_params) {
    const auto &configs = app_configs.configs();
    gen_pir_params(configs.number_of_elements(), configs.size_per_element(),
                   configs.dimensions(), enc_params, pir_params, configs.use_symmetric(),
                   configs.use_batching(), configs.use_recursive_mod_switching());

    std::vector<PIRClient> clients;
    clients.reserve(size);

    concurrency::threadpool pool;
    auto latch = std::make_shared<concurrency::safelatch>(size);
    std::mutex mtx;
    for (size_t i = 0; i < size; i++) {
        pool.submit(
            {
                .f = [&, i]() {
                    auto tmp = PIRClient(enc_params, pir_params);
                    mtx.lock();
                    clients.push_back(std::move(tmp));
                    mtx.unlock();
                },
                .wg = latch
            }
        );
    }
    latch->wait();

    return clients;
}


std::map<uint32_t, std::unique_ptr<services::ClientInfo>>
create_client_db(const distribicom::AppConfigs &app_configs, const seal::EncryptionParameters &enc_params,
                 PirParams &pir_params, std::vector<PIRClient> &clients) {
    seal::SEALContext seal_context(enc_params, true);
    const auto &configs = app_configs.configs();
    gen_pir_params(configs.number_of_elements(), configs.size_per_element(),
                   configs.dimensions(), enc_params, pir_params, configs.use_symmetric(),
                   configs.use_batching(), configs.use_recursive_mod_switching());

    std::map<uint32_t, std::unique_ptr<services::ClientInfo>> cdb;
    auto m = marshal::Marshaller::Create(enc_params);

    concurrency::threadpool pool;
    auto latch = std::make_shared<concurrency::safelatch>(clients.size());
    std::mutex mtx;
    for (size_t i = 0; i < clients.size(); i++) {
        pool.submit(
            {
                .f = [&, i]() {
                    seal::GaloisKeys gkey = clients[i].generate_galois_keys();
                    auto gkey_serialised = m->marshal_seal_object(gkey);
                    PirQuery query = clients[i].generate_query(i % (configs.db_rows() * configs.db_cols()));
                    distribicom::ClientQueryRequest query_marshaled;
                    m->marshal_query_vector(query, query_marshaled);
                    auto client_info = std::make_unique<services::ClientInfo>(services::ClientInfo());

                    services::set_client(
                        math_utils::compute_expansion_ratio(seal_context.first_context_data()->parms()) * 2,
                        app_configs.configs().db_rows(), i, gkey, gkey_serialised, query, query_marshaled,
                        client_info);

                    mtx.lock();
                    cdb.insert({i, std::move(client_info)});
                    mtx.unlock();
                },
                .wg = latch,
            }
        );
    }
    latch->wait();
    return cdb;
}


shared_ptr<services::FullServer>
full_server_instance(const distribicom::AppConfigs &configs, const seal::EncryptionParameters &enc_params,
                     PirParams &pir_params, std::vector<PIRClient> &clients) {
    auto cols = configs.configs().db_cols();
    auto rows = configs.configs().db_rows();

    math_utils::matrix<seal::Plaintext> db(rows, cols);
    std::uint64_t i = 0;
    for (auto &ptx: db.data) {
        ptx = i++;
    }
    auto client_db = create_client_db(configs, enc_params, pir_params, clients);
    return std::make_shared<services::FullServer>(db, client_db, configs);
}
