#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"


namespace services::configurations {

    distribicom::Configs create_configs(int poly_deg, int logt, int rows, int cols, std::uint64_t ele_size);

    distribicom::AppConfigs
    create_app_configs(const std::string &server_hostname, int poly_deg, int logt, int rows, int cols, std::uint64_t
    ele_size, std::uint64_t number_of_workers, std::uint64_t query_wait_time, std::uint64_t number_of_clients);

//    distribicom::WorkerConfigs create_worker_configs(const distribicom::AppConfigs &app_configs, int worker_port, std::string worker_hostname);
//
    distribicom::ClientConfigs create_client_configs(const distribicom::AppConfigs &app_configs, int client_port);
}
