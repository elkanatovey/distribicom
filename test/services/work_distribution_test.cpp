#include "../test_utils.hpp"
#include "worker.hpp"
#include "factory.hpp"
#include "server.hpp"
#include <grpc++/grpc++.h>
#include <latch>

// TODO: do not copy from worker_test, reuse code.
std::map<uint32_t, std::unique_ptr<services::ClientInfo>>
create_client_db(int size, std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &app_configs);

int work_distribution_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto cfgs = services::configurations::create_app_configs(
        "localhost:122345",
        int(all->encryption_params.poly_modulus_degree()),
        20,
        3,
        3,
        256
    );
    auto num_workers = 9;
    auto num_queries = 18;

    math_utils::matrix<seal::Plaintext> db(cfgs.configs().db_rows(), cfgs.configs().db_rows());

    for (auto &p: db.data) {
        p = all->random_plaintext();
    }

    auto cdb = create_client_db(1, all, cfgs);
    //const distribicom::AppConfigs &app_configs, std::map<uint32_t, std::unique_ptr<services::ClientInfo>> &client_db, math_utils::matrix<seal::Plaintext> &db
    services::Manager manager(cfgs, cdb, db);

    for (int i = 0; i < num_workers; ++i) {
        manager.work_streams[std::to_string(i)] = nullptr;
    }

    manager.map_workers_to_responsibilities2(num_queries);

    /* todo:
         verify each group has the same number of queries.
         verify each group holds all DB.
         verify each group has different workers.
         verify each group has different queries.
     */
    return 0;
}
