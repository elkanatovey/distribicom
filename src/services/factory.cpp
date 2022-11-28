#include "factory.hpp"

distribicom::Configs services::configurations::create_configs(int poly_deg, int logt, int rows, int cols) {
    distribicom::Configs c;
    c.mutable_scheme()->assign("bgv");
    c.set_polynomial_degree(poly_deg);
    c.set_logarithm_plaintext_coefficient(logt);
    c.set_db_rows(rows);
    c.set_db_cols(cols);

    c.set_polynomial_degree(poly_deg);
    return c;
}

distribicom::AppConfigs
services::configurations::create_app_configs(const std::string &server_hostname, int poly_deg, int logt, int rows,
                                             int cols) {
    distribicom::AppConfigs c;
    c.mutable_configs()->CopyFrom(create_configs(poly_deg, logt, rows, cols));
    c.mutable_main_server_hostname()->assign(server_hostname);

    return c;
}

distribicom::WorkerConfigs
services::configurations::create_worker_configs(const distribicom::AppConfigs &app_configs, int worker_port) {
    distribicom::WorkerConfigs c;
    c.set_workerport(worker_port);
    c.mutable_appconfigs()->CopyFrom(app_configs);
    return c;
}

distribicom::ClientConfigs
services::configurations::create_client_configs(const distribicom::AppConfigs &app_configs, int client_port) {
    distribicom::ClientConfigs c;
    c.set_client_port(client_port);
    c.mutable_app_configs()->CopyFrom(app_configs);
    return c;
}
