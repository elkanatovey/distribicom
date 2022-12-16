#include <iostream>
#include <filesystem>
#include "worker.hpp"
#include "marshal/local_storage.hpp"

// TODO: understand how to use export to different file.
bool is_valid_command_line_args(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <config_file>" << std::endl;
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

    distribicom::AppConfigs cnfgs;
    cnfgs.ParseFromString(load_from_file(argv[1]));

    services::Worker w(std::move(cnfgs));
    std::cout << "waiting for stream termination" << std::endl;
    auto status = w.wait_for_stream_termination();
    if (!status.ok()) {
        std::cout << "terminated stream, result:" << status.error_message() << std::endl;;
    }

    w.close();
}