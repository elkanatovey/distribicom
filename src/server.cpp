#include <iostream>
#include "services/factory.hpp"
#include "marshal/local_storage.hpp"
#include "server.hpp"

std::string get_listening_address(const distribicom::AppConfigs &cnfgs);

shared_ptr<services::FullServer>
full_server_instance(const distribicom::AppConfigs &configs);

std::thread run_server(const latch &, shared_ptr<services::FullServer>, distribicom::AppConfigs &);

int main(int, char *[]) {
    // todo: load configs from config file in specific folder.
    auto cnfgs = services::configurations::create_app_configs("0.0.0.0:5432", 4096, 20, 5, 5, 256);
    cnfgs.set_number_of_workers(5); // todo: should load with this value from config file.
    auto server = full_server_instance(cnfgs);

    std::cout << "setting server" << std::endl;
    std::latch wg(1);
    std::vector<std::thread> threads;
    threads.emplace_back(run_server(wg, server, cnfgs));

    server->wait_for_workers(int(cnfgs.number_of_workers()));
    // 1 epoch
    for (int i = 0; i < 1; ++i) {
        server->start_epoch();

        // 5 rounds in an epoch.
        for (int j = 0; j < 10; ++j) {
            // todo: start timing a full round here.
            auto ledger = server->distribute_work();

            ledger->done.read_for(std::chrono::milliseconds(5000)); // todo: how much time should we wait?

            // server should now inspect missing items and run the calculations for them.
            // server should also be notified by ledger about the rouge workers.
            server->learn_about_rouge_workers(ledger);

            // perform step 2.
            server->run_step_2(ledger);

//            server->publish_answers();
        }
    }

    server->send_stop_signal();
    wg.count_down();
    for (auto &t: threads) {
        t.join();
    }
    std::cout << "done." << std::endl;
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

shared_ptr<services::FullServer> full_server_instance(const distribicom::AppConfigs &configs) {
    auto cols = configs.configs().db_cols();
    auto rows = configs.configs().db_rows();

    math_utils::matrix<seal::Plaintext> db(rows, cols);
    std::uint64_t i = 0;
    for (auto &ptx: db.data) {
        ptx = i++;
    }
    std::map<uint32_t, std::unique_ptr<services::ClientInfo>> client_db;
    std::uint64_t max_clients = 1 << 16;
    for (std::uint64_t j = 0; j < max_clients; ++j) {
        // TODO: create queries.
        client_db.insert(
            {
                j,
                std::make_unique<services::ClientInfo>(),
            }
        );
    }
    return std::make_shared<services::FullServer>(db, client_db, configs);
}
