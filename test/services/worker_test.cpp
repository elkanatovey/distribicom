#include "../test_utils.hpp"
#include "worker.hpp"
#include <grpc++/grpc++.h>


std::unique_ptr<grpc::Server> run_server();

int worker_test(int, char *[]) {
    WaitGroup wg;

    wg.add(1);
    std::thread t([&] {
        std::string server_address("0.0.0.0:50051");
        services::Worker service;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        auto server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;
        std::cout << "\n\n" << std::endl;
        wg.wait();
        server->Shutdown();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * 1000));

    distribicom::Worker::Stub client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    grpc::ClientContext context;
    distribicom::Ack response;
    distribicom::MatrixPart m;

    services::add_matrix_size(context, 100);
    auto conn = client.SendTasks(&context, &response);

    conn->WriteLast(m, grpc::WriteOptions{});
    auto out = conn->Finish();
    std::cout << "status:" << out.ok() << std::endl;
    wg.done();
    t.join();
    return 0;
}

std::unique_ptr<grpc::Server> run_server() {
    std::string server_address("0.0.0.0:50051");
    services::Worker service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    std::cout << "\n\n" << std::endl;
    server->Wait();
    return nullptr;
}