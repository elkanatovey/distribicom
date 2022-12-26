#include "../test_utils.hpp"
#include "worker.hpp"
#include "factory.hpp"
#include "server.hpp"
#include <grpc++/grpc++.h>
#include <latch>

// TODO: do not copy from worker_test, reuse code.
std::map<uint32_t, std::unique_ptr<services::ClientInfo>>
create_client_db(int size, std::shared_ptr<TestUtils::CryptoObjects> &all, const distribicom::AppConfigs &app_configs);

void verify_partitions_hold_the_same_amount_of_work(map<string, services::WorkerInfo> &partition);

std::vector<services::WorkerInfo>
split_partitions_by_group(map<string, services::WorkerInfo> &partitions, uint64_t grp_id);

std::set<std::uint64_t> get_group_ids(map<string, services::WorkerInfo> &partitions);

void verify_workers_in_group_contain_the_full_db(map<string, services::WorkerInfo> &partitions,
                                                 distribicom::AppConfigs &app_cfgs);

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

    auto partitions = manager.map_workers_to_responsibilities2(num_queries);

    /* todo:
         verify each group has different workers.
         verify each group has different queries.
         verify all groups together have all queries.
     */

    verify_partitions_hold_the_same_amount_of_work(partitions);
    verify_workers_in_group_contain_the_full_db(partitions, cfgs);
    return 0;
}

void
verify_workers_in_group_contain_the_full_db(map<string, services::WorkerInfo> &partitions,
                                            distribicom::AppConfigs &app_cfgs) {
    auto grp_ids = get_group_ids(partitions);
    for (const auto &grp_id: grp_ids) {
        auto splits = split_partitions_by_group(partitions, grp_id);
        if (splits.empty()) {
            throw std::runtime_error("no splits for group");
        }

        std::set<uint64_t> db_rows;
        for (std::uint64_t i = 0; i < app_cfgs.configs().db_rows(); ++i) {
            db_rows.insert(i);
        }

        for (const auto &worker_info: splits) {
            for (const auto &row: worker_info.db_rows) {
                assert(db_rows.find(row) != db_rows.end()); // ensure row presents.s
                db_rows.erase(row);
            }
        }
        assert(db_rows.empty()); // ensures each group covers the DB fully.
    }
}

std::set<std::uint64_t> get_group_ids(map<string, services::WorkerInfo> &partitions) {
    std::set<std::uint64_t> group_ids;
    for (const auto &[nm, info]: partitions) {
        (void) nm;
        group_ids.insert(info.group_number);
    }
    return group_ids;
}

std::vector<services::WorkerInfo>
split_partitions_by_group(map<string, services::WorkerInfo> &partitions, uint64_t grp_id) {
    std::vector<services::WorkerInfo> splits;
    for (const auto &[nm, info]: partitions) {
        (void) nm;

        if (info.group_number != grp_id) {
            continue;
        }

        splits.push_back(info);
    }
    return splits;
}

void verify_partitions_hold_the_same_amount_of_work(map<string, services::WorkerInfo> &partition) {
    auto grp_ids = get_group_ids(partition);
    for (const auto &grp_id: grp_ids) {
        auto splits = split_partitions_by_group(partition, grp_id);
        if (splits.empty()) {
            throw std::runtime_error("no splits for group");
        }

        auto query_range_start = splits[0].query_range_start;
        auto query_range_end = splits[0].query_range_end;
        for (const auto &worker_info: splits) {
            assert(worker_info.query_range_start == query_range_start);
            assert(worker_info.query_range_end == query_range_end);
        }
    }
}
