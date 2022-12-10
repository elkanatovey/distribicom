//
// Created by Jonathan Weiss on 12/10/22.
//

#include "threadpool.hpp"

concurrency::threadpool::threadpool(uint64_t n_threads) : chan() {
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
