#include <iostream>
#include "threadpool.hpp"
#include "globals.h"

namespace concurrency {
    std::uint64_t num_cpus = std::thread::hardware_concurrency();
}

concurrency::threadpool::threadpool(uint64_t n_threads) : chan() {
    std::cout << "creating threadpool with " << n_threads << " threads" << std::endl;
    for (uint64_t i = 0; i < n_threads; ++i) {
        threads.emplace_back([this]() {
            while (true) {
                auto task = chan.read();
                if (!task.ok) {
                    return; // closed channel.
                }
                task.answer.f();
                task.answer.wg->count_down();
            }
        });
    }
}

concurrency::threadpool::~threadpool() {
    chan.close();
    for (auto &t: threads) {
        t.join();
    }
}

void concurrency::threadpool::submit(concurrency::Task task) {
    chan.write(task);
}

concurrency::threadpool::threadpool() : threadpool(num_cpus) {}
