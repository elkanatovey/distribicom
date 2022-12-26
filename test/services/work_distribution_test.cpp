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

void verify_disjoint_groups(map<string, services::WorkerInfo> &partitions);

void verify_each_group_has_different_queries(map<string, services::WorkerInfo> &partitions);

void verify_all_queries_are_covered(map<string, services::WorkerInfo> &partitions, std::uint64_t num_queries);

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

    math_utils::matrix<seal::Plaintext> db(0, 0);

    auto cdb = create_client_db(1, all, cfgs);
    //const distribicom::AppConfigs &app_configs, std::map<uint32_t, std::unique_ptr<services::ClientInfo>> &client_db, math_utils::matrix<seal::Plaintext> &db
    services::Manager manager(cfgs, cdb, db);

    for (int i = 0; i < num_workers; ++i) {
        manager.work_streams[std::to_string(i)] = nullptr;
    }

    auto partitions = manager.map_workers_to_responsibilities2(num_queries);


    verify_partitions_hold_the_same_amount_of_work(partitions);
    verify_workers_in_group_contain_the_full_db(partitions, cfgs);
    verify_disjoint_groups(partitions);
    verify_each_group_has_different_queries(partitions);
    verify_all_queries_are_covered(partitions, num_queries);
    return 0;
}

void verify_all_queries_are_covered(map<string, services::WorkerInfo> &partitions, std::uint64_t num_queries) {
    std::set<std::uint64_t> queries;
    for (auto grp_id: get_group_ids(partitions)) {
        auto workers = split_partitions_by_group(partitions, grp_id);

        for (auto i = workers[0].query_range_start; i < workers[0].query_range_end; ++i) {
            queries.insert(i);
        }
    }
    assert(queries.size() == num_queries);
}

void verify_each_group_has_different_queries(map<string, services::WorkerInfo> &partitions) {
    auto grps = get_group_ids(partitions);
    std::map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>> id_to_query_start_end;
    for (auto grp_id: grps) {
        auto splits = split_partitions_by_group(partitions, grp_id);
        assert(!splits.empty());
        auto worker = splits[0];
        id_to_query_start_end.insert({grp_id, {worker.query_range_start, worker.query_range_end}});
    }

    for (auto &grp_id1: grps) {
        for (auto &grp_id2: grps) {
            if (grp_id1 == grp_id2) {
                continue;
            }
            auto query_start_end1 = id_to_query_start_end[grp_id1];
            auto query_start_end2 = id_to_query_start_end[grp_id2];
            assert(query_start_end1.first != query_start_end2.first);
            assert(query_start_end1.second != query_start_end2.second);
        }
    }

}

// assumes each worker has a different number.
void verify_disjoint_groups(map<string, services::WorkerInfo> &partitions) {
    std::set<std::uint64_t> workers;

    for (auto &group_id: get_group_ids(partitions)) {
        auto group = split_partitions_by_group(partitions, group_id);
        assert(!group.empty());

        for (auto &worker: group) {
            assert(workers.find(worker.worker_number) == workers.end());
            workers.insert(worker.worker_number);
        }
    }
}

void
verify_workers_in_group_contain_the_full_db(map<string, services::WorkerInfo> &partitions,
                                            distribicom::AppConfigs &app_cfgs) {
    for (const auto &grp_id: get_group_ids(partitions)) {
        auto splits = split_partitions_by_group(partitions, grp_id);
        assert(!splits.empty());

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
    for (const auto &grp_id: get_group_ids(partition)) {
        auto splits = split_partitions_by_group(partition, grp_id);
        assert(!splits.empty());

        auto query_range_start = splits[0].query_range_start;
        auto query_range_end = splits[0].query_range_end;
        for (const auto &worker_info: splits) {
            assert(worker_info.query_range_start == query_range_start);
            assert(worker_info.query_range_end == query_range_end);
        }
    }
}
