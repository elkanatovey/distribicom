#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"


namespace services {

    distribicom::Configs create_configs(int poly_deg, int logt, int rows, int cols) {
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
    create_app_configs(const std::string &server_hostname, int poly_deg, int logt, int rows, int cols) {
        distribicom::AppConfigs c;
        c.mutable_configs()->CopyFrom(create_configs(poly_deg, logt, rows, cols));
        c.mutable_main_server_hostname()->assign(server_hostname);

        return c;
    }

}
