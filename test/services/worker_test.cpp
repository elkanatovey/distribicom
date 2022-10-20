#include "../test_utils.hpp"
#include "worker.hpp"
#include "factory.hpp"
#include <grpc++/grpc++.h>


std::unique_ptr<grpc::Server> run_server();

int worker_test(int, char *[]) {
    auto cfgs = services::create_app_configs("srvr", 4096, 20, 50, 50);

    WaitGroup wg;

    wg.add(1);
    std::thread t([&] {
        std::string server_address("0.0.0.0:50051");
        services::Worker service(cfgs);

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


    services::add_matrix_size(context, 100);
    auto conn = client.SendTasks(&context, &response);

    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);
    auto ptx = all->random_plaintext();

    auto marshaler = marshal::Marshaller::Create(all->encryption_params);


    distribicom::Plaintext mptx;
    mptx.mutable_data()->assign(marshaler->marshal_seal_object<seal::Plaintext>(ptx));
    distribicom::MatrixPart m;
    m.mutable_ptx()->CopyFrom(mptx);
    for (int i = 0; i < 5; ++i) {
        m.set_col(i);
        m.set_row(0);
        conn->Write(m, grpc::WriteOptions{});
    }
    conn->WritesDone();
    auto out = conn->Finish();
    std::cout << "status:" << out.ok() << std::endl;
    wg.done();
    t.join();
    return 0;
}

std::unique_ptr<grpc::Server> run_server() {
    std::string server_address("0.0.0.0:50051");
    distribicom::AppConfigs a;
    services::Worker service(a);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    std::cout << "\n\n" << std::endl;
    server->Wait();
    return nullptr;
}