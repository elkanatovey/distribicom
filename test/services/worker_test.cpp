#include "../test_utils.hpp"
#include "worker.hpp"
#include "factory.hpp"
#include "server.hpp"
#include <grpc++/grpc++.h>
#include <latch>

constexpr std::string_view server_port = "5051";
constexpr std::string_view worker_port = "52100";

std::thread runFullServer(std::latch &wg, services::FullServer &f);

std::thread setupWorker(std::latch &wg, distribicom::AppConfigs &configs);

services::FullServer
full_server_instance(std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &configs);

void sleep(int n_seconds) { std::this_thread::sleep_for(std::chrono::milliseconds(n_seconds * 1000)); }

int worker_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto cfgs = services::configurations::create_app_configs(
        "localhost:" + std::string(server_port),
        int(all->encryption_params.poly_modulus_degree()),
        20,
        6,
        6,
        256
    );
    services::FullServer fs = full_server_instance(all, cfgs);

    std::latch wg(1);

    std::vector<std::thread> threads;
    threads.emplace_back(runFullServer(wg, fs));

    std::cout << "setting up manager and client services." << std::endl;
    sleep(3);

    std::cout << "setting up worker-service" << std::endl;
    threads.emplace_back(setupWorker(wg, cfgs));

    fs.wait_for_workers(1);
    fs.start_epoch();
    fs.learn_about_rouge_workers(fs.distribute_work());

    sleep(5);
    std::cout << "\nshutting down.\n" << std::endl;
    wg.count_down();
    for (auto &t: threads) {
        t.join();
    }
    sleep(5);
    return 0;
}


std::map<uint32_t, std::unique_ptr<services::ClientInfo>>
create_client_db(int size, std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &app_configs) {
    auto m = marshal::Marshaller::Create(all->encryption_params);
    std::map<uint32_t, std::unique_ptr<services::ClientInfo>> cdb;
    for (int i = 0; i < size; i++) {
        auto gkey = all->gal_keys;
        auto gkey_serialised = m->marshal_seal_object(gkey);
        std::vector<std::vector<seal::Ciphertext>> query = {{all->random_ciphertext()},
                                                            {all->random_ciphertext()}};
        distribicom::ClientQueryRequest query_marshaled;
        m->marshal_query_vector(query, query_marshaled);
        auto client_info = std::make_unique<services::ClientInfo>(services::ClientInfo());

        services::set_client(math_utils::compute_expansion_ratio(all->seal_context.last_context_data()->parms()) * 2,
                             app_configs.configs().db_rows(), i, gkey, gkey_serialised, query, query_marshaled,
                             client_info);

        cdb.insert(
            {i, std::move(client_info)});
    }
    return cdb;
}

services::FullServer
full_server_instance(std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &configs) {
    auto num_clients = configs.configs().db_rows() * 2;
    math_utils::matrix<seal::Plaintext> db(configs.configs().db_rows(), configs.configs().db_rows());

    for (auto &p: db.data) {
        p = all->random_plaintext();
    }

    auto cdb = create_client_db(num_clients, all, configs);

    return services::FullServer(db, cdb, configs);
}


// assumes that configs are not freed until we copy it inside the thread!
std::thread setupWorker(std::latch &wg, distribicom::AppConfigs &configs) {
    return std::thread([&] {
        try {
            services::Worker worker(
                services::configurations::create_worker_configs(
                    configs,
                    std::stoi(std::string(worker_port)),
                    "0.0.0.0"
                )
            );

            wg.wait();
            worker.close();
        } catch (std::exception &e) {
            std::cerr << "setupWorker :: exception: " << e.what() << std::endl;
        }
    });
}

std::thread runFullServer(std::latch &wg, services::FullServer &f) {
    return std::thread([&] {
        std::string server_address("0.0.0.0:" + std::string(server_port));


        grpc::ServerBuilder builder;
        builder.SetMaxMessageSize(services::constants::max_message_size);

        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

        auto sv = f.get_manager_service();
        std::cout << "had sync methods?" << sv->has_synchronous_methods() << std::endl;
        std::cout << "had callback methods?" << sv->has_callback_methods() << std::endl;
        std::cout << "had generic methods?" << sv->has_generic_methods() << std::endl;

        builder.RegisterService(f.get_manager_service());
        auto server(builder.BuildAndStart());
        std::cout << "manager and client services are listening on " << server_address << std::endl;
        wg.wait();

        f.close();

        server->Shutdown();
    });
}