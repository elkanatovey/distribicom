#include "../test_utils.hpp"
#include "worker.hpp"
#include "factory.hpp"
#include "server.hpp"
#include <grpc++/grpc++.h>

constexpr std::string_view server_port = "5051";
constexpr std::string_view worker_port = "52100";

std::thread runFullServer(concurrency::WaitGroup &wg, services::FullServer &f);

std::thread setupWorker(concurrency::WaitGroup &wg, distribicom::AppConfigs &configs);

services::FullServer
full_server_instance(std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &configs);

void sleep(int n_seconds) { std::this_thread::sleep_for(std::chrono::milliseconds(n_seconds * 1000)); }

int worker_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto cfgs = services::configurations::create_app_configs(
            "localhost:" + std::string(server_port),
            int(all->encryption_params.poly_modulus_degree()),
            20,
            50,
            50,
            256
    );
    services::FullServer fs = full_server_instance(all, cfgs);

    concurrency::WaitGroup wg;
    wg.add(1); // will tell the servers when to quit.

    std::vector<std::thread> threads;
    threads.emplace_back(runFullServer(wg, fs));

    std::cout << "setting up manager and client services." << std::endl;
    sleep(3);

    std::cout << "setting up worker-service" << std::endl;
    threads.emplace_back(setupWorker(wg, cfgs));
    sleep(2);

    fs.wait_for_workers(1);
    fs.start_epoch();
    fs.distribute_work();

    sleep(5);
    std::cout << "\nshutting down.\n" << std::endl;
    wg.done();
    for (auto &t: threads) {
        t.join();
    }
    sleep(5);
    return 0;
}

services::FullServer
full_server_instance(std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &configs) {
    auto n = 5;
    math_utils::matrix<seal::Plaintext> db(n, n);
    // a single row of ctxs and their respective gal_key.
    math_utils::matrix<seal::Ciphertext> queries(1, n);
    math_utils::matrix<seal::GaloisKeys> gal_keys(1, n);
    for (auto &p: db.data) {
        p = all->random_plaintext();
    }

    for (auto &q: queries.data) {
        q = all->random_ciphertext();
    }

    for (auto &g: gal_keys.data) {
        g = all->gal_keys;
    }

    return services::FullServer(db, queries, gal_keys, configs);
}


// assumes that configs are not freed until we copy it inside the thread!
std::thread setupWorker(concurrency::WaitGroup &wg, distribicom::AppConfigs &configs) {
    return std::thread([&] {
        try {
            services::Worker worker(
                    services::configurations::create_worker_configs(
                            configs,
                            std::stoi(std::string(worker_port))
                    )
            );

            grpc::ServerBuilder builder;

            builder.SetMaxMessageSize(services::constants::max_message_size);

            std::string server_address("0.0.0.0:" + std::string(worker_port));
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&worker);

            auto server(builder.BuildAndStart());
            std::cout << "worker-service listening on " << server_address << std::endl;
            wg.wait();
            server->Shutdown();
            worker.close();
        } catch (std::exception &e) {
            std::cerr << "setupWorker :: exception: " << e.what() << std::endl;
        }
    });
}

std::thread runFullServer(concurrency::WaitGroup &wg, services::FullServer &f) {
    return std::thread([&] {
        std::string server_address("0.0.0.0:" + std::string(server_port));


        grpc::ServerBuilder builder;
        builder.SetMaxMessageSize(services::constants::max_message_size);

        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());


        builder.RegisterService(f.get_manager_service());
        auto server(builder.BuildAndStart());
        std::cout << "manager and client services are listening on " << server_address << std::endl;
        wg.wait();
        server->Shutdown();
    });
}