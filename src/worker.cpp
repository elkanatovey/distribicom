#include <iostream>
#include <filesystem>
#include "worker.hpp"
#include "marshal/local_storage.hpp"
#include "services/factory.hpp"

// TODO: understand how to use export to different file.
bool is_valid_command_line_args(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <pir_config_file>" << " <num_worker_threads>" << " <server_address:server_listening_port>" << std::endl;
        return false;
    }

    if (!std::filesystem::exists(argv[1])) {
        std::cout << "Config file " << argv[1] << " does not exist" << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (!is_valid_command_line_args(argc, argv)) {
        return -1;
    }

    auto pir_conf_file = argv[1];
    auto num_worker_threads = std::stoi(argv[2]);
    auto server_address = argv[3];

    distribicom::Configs temp_pir_configs;
    auto json_str = load_from_file(pir_conf_file);
    google::protobuf::util::JsonStringToMessage(load_from_file(pir_conf_file), &temp_pir_configs);
    distribicom::AppConfigs cnfgs = services::configurations::create_app_configs(server_address, temp_pir_configs
                                                                                         .polynomial_degree(),
                                                                                 temp_pir_configs.logarithm_plaintext_coefficient(),
                                                                                 temp_pir_configs.db_rows(),
                                                                                 temp_pir_configs.db_cols(),
                                                                                 temp_pir_configs.size_per_element(),
                                                                                 1,
                                                                                 10, 1);

    cnfgs.set_worker_num_cpus(num_worker_threads); //@todo refactor into creation func


    if (cnfgs.worker_num_cpus() > 0) {
        concurrency::num_cpus = cnfgs.worker_num_cpus();
        std::cout << "set global num cpus to:" << concurrency::num_cpus << std::endl;
    }


    services::Worker w(std::move(cnfgs));
    std::cout << "waiting for stream termination" << std::endl;
    auto status = w.wait_for_stream_termination();
    if (!status.ok()) {
        std::cout << "terminated stream, result:" << status.error_message() << std::endl;;
    }

    w.close();
}