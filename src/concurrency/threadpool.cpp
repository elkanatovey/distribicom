#include <iostream>
#include <seal/seal.h>
#include "threadpool.hpp"
#include "globals.h"

namespace concurrency {
    std::uint64_t num_cpus = std::thread::hardware_concurrency();

    std::map<std::thread::id, seal::MemoryPoolHandle> thread_local_pool;
    std::mutex thread_local_pool_mutex;

    seal::MemoryPoolHandle threadpool::GetSealPoolHandle() {
        auto id = std::this_thread::get_id();

        std::unique_lock lck(thread_local_pool_mutex);

        if (thread_local_pool.find(id) == thread_local_pool.end()) {
            std::cerr << "memory-pool of thread " << id << " not found" << std::endl;
            thread_local_pool[id] = std::move(seal::MemoryPoolHandle::New());
        }

        // using copy constructor
        return {thread_local_pool[id]};
    }


    threadpool::threadpool(uint64_t n_threads) : chan() {
        std::cout << "creating threadpool with " << n_threads << " threads" << std::endl;
        for (uint64_t i = 0; i < n_threads; ++i) {
            threads.emplace_back([this]() {

                auto thread_id = std::this_thread::get_id();

                thread_local_pool_mutex.lock();
                thread_local_pool[thread_id] = std::move(seal::MemoryPoolHandle::New());
                thread_local_pool_mutex.unlock();

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