#include <iostream>
#include "services/factory.hpp"
#include "worker.hpp"

int main(int, char *[]) {
    // todo: load configs from config file in specific folder.
    // todo: need to define how to find server's ip address... it should be in a specific file.
    // maybe when the server sets itself up it creates this file which everyone will read from?
    auto server_ip = "0.0.0.0:5432";
    auto wcnfgs = services::configurations::create_worker_configs(
        services::configurations::create_app_configs(server_ip, 4096, 20, 5, 5, 256),
        std::stoi(std::string("0")),
        server_ip
    );

    services::Worker w(std::move(wcnfgs));
    w.wait_for_stream_termination();
    w.close();
}