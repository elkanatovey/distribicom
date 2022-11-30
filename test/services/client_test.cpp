#include "../test_utils.hpp"
#include "worker.hpp"
#include "factory.hpp"
#include "server.hpp"
#include "client_service.hpp"
#include <grpc++/grpc++.h>

constexpr std::string_view server_port_client_test = "5051";
constexpr std::string_view worker_port_client_test = "52100";
constexpr std::string_view client_port_client_test = "521001";

std::thread startFullServer(concurrency::WaitGroup &wg, services::FullServer &f);

std::thread startupWorker(concurrency::WaitGroup &wg, distribicom::AppConfigs &configs);

std::thread setupClient(concurrency::WaitGroup &wg, distribicom::AppConfigs &configs);

services::FullServer
full_server_struct(std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &configs);

void sleep_client_test(int n_seconds) { std::this_thread::sleep_for(std::chrono::milliseconds(n_seconds * 1000)); }


int client_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto cfgs = services::configurations::create_app_configs(
            "localhost:" + std::string(server_port_client_test),
            int(all->encryption_params.poly_modulus_degree()),
            20,
            50,
            50,
            256
    );
    services::FullServer fs = full_server_struct(all, cfgs);

    concurrency::WaitGroup wg;
    wg.add(1); // will tell the servers when to quit.

    std::vector<std::thread> threads;
    threads.emplace_back(startFullServer(wg, fs));

    std::cout << "setting up manager and client services." << std::endl;
    sleep_client_test(3);

    std::cout << "setting up worker-service" << std::endl;
    threads.emplace_back(startupWorker(wg, cfgs));
    sleep_client_test(3);

    std::cout << "setting up client-service" << std::endl;
    threads.emplace_back(setupClient(wg, cfgs));
    sleep_client_test(3);
    fs.distribute_work();



//    distribicom::Worker::Stub client(
//            grpc::CreateChannel("localhost:" + std::string(worker_port_client_test), grpc::InsecureChannelCredentials()));
//
//    grpc::ClientContext context;
//    distribicom::Ack response;
//
//
//    services::add_metadata_size(context, services::constants::size_md, 5);
//    services::add_metadata_size(context, services::constants::round_md, 1);
//    services::add_metadata_size(context, services::constants::epoch_md, 2);
//
//    auto conn = client.SendTask(&context, &response);
//
//    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);
//    auto ptx = all->random_plaintext();
//
//    auto marshaler = marshal::Marshaller::Create(all->encryption_params);
//
//
//    distribicom::Plaintext mptx;
//    mptx.mutable_data()->assign(marshaler->marshal_seal_object<seal::Plaintext>(ptx));
//    distribicom::MatrixPart m;
//    m.mutable_ptx()->CopyFrom(mptx);
//
//    distribicom::WorkerTaskPart tsk;
//    tsk.mutable_matrixpart()->CopyFrom(m);
//    tsk.mutable_matrixpart()->set_row(0);
//
//    for (int i = 0; i < 5; ++i) {
//        tsk.mutable_matrixpart()->set_col(i);
//        conn->Write(tsk, grpc::WriteOptions{});
//    }
//    conn->WritesDone();
//    auto out = conn->Finish();
//    assert(out.ok() == 1);


    std::cout << "\nshutting down.\n" << std::endl;
    wg.done();
    for (auto &t: threads) {
        t.join();
    }

    return 0;
}

services::FullServer
full_server_struct(std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &configs) {
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
std::thread setupClient(concurrency::WaitGroup &wg, distribicom::AppConfigs &configs) {
    return std::thread([&] {
        try {
            services::ClientListener client(
                    services::configurations::create_client_configs(
                            configs,
                            std::stoi(std::string(client_port_client_test))
                    )
            );

            grpc::ServerBuilder builder;

            std::string server_address("0.0.0.0:" + std::string(client_port_client_test));
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&client);

            auto server(builder.BuildAndStart());
            std::cout << "client-service listening on " << server_address << std::endl;
            wg.wait();
            server->Shutdown();
        } catch (std::exception &e) {
            std::cerr << "setupClient :: exception: " << e.what() << std::endl;
        }
    });
}


// assumes that configs are not freed until we copy it inside the thread!
std::thread startupWorker(concurrency::WaitGroup &wg, distribicom::AppConfigs &configs) {
    return std::thread([&] {
        try {
            services::Worker worker(
                    services::configurations::create_worker_configs(
                            configs,
                            std::stoi(std::string(worker_port_client_test))
                    )
            );

            grpc::ServerBuilder builder;

            std::string server_address("0.0.0.0:" + std::string(worker_port_client_test));
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&worker);

            auto server(builder.BuildAndStart());
            std::cout << "worker-service listening on " << server_address << std::endl;
            wg.wait();
            server->Shutdown();
        } catch (std::exception &e) {
            std::cerr << "startupWorker :: exception: " << e.what() << std::endl;
        }
    });
}

std::thread startFullServer(concurrency::WaitGroup &wg, services::FullServer &f) {
    return std::thread([&] {
        std::string server_address("0.0.0.0:" + std::string(server_port_client_test));


        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());


        builder.RegisterService(f.get_manager_service());
        auto server(builder.BuildAndStart());
        std::cout << "manager and client services are listening on " << server_address << std::endl;
        wg.wait();
        server->Shutdown();
    });
}