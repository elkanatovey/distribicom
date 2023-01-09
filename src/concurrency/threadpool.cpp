#include <iostream>
#include <seal/seal.h>
#include "threadpool.hpp"
#include "globals.h"

namespace concurrency {
    std::uint64_t num_cpus = std::thread::hardware_concurrency();

    threadpool::threadpool(uint64_t n_threads) : chan() {
        std::cout << "creating threadpool with " << n_threads << " threads" << std::endl;
        for (uint64_t i = 0; i < n_threads; ++i) {
            threads.emplace_back([this]() {
                while (true) {
                    auto task = chan.read();
                    if (!task.ok) {
                        return; // closed channel.
                    }

                    try {
                        task.answer.f();
                    } catch (std::exception &e) {
                        std::cerr << "threadpool exception::" << task.answer.name << ":" << e.what() << std::endl;
                    }

                    if (task.answer.wg != nullptr) {
                        task.answer.wg->count_down();
                    }
                }
            });
        }
    }

    threadpool::~threadpool() {
        chan.close();
        for (auto &t: threads) {
            t.join();
        }
    }


    threadpool::threadpool() : threadpool(num_cpus) {}
}