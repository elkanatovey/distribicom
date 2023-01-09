#pragma once

#include <thread>
#include <vector>
#include <latch>
#include <functional>
#include <memory>
#include "channel.hpp"
#include "safelatch.h"
#include "globals.h"

namespace concurrency {
    struct Task {
        std::function<void()> f;
        std::shared_ptr<safelatch> wg;
        std::string name;
    };


    class threadpool {
    public:
        explicit threadpool();;

        explicit threadpool(uint64_t n_threads);

        ~threadpool();

        inline void submit(Task &&task) { chan.write(std::move(task)); }

        static seal::MemoryPoolHandle GetSealPoolHandle() {
            // the map that holds the pools as a start should be thread safe: locked with mutex on every write.
            // and readsafe using shared_mutex.

            // held by unique_ptr at the global level throughout the whole program
            std::this_thread::get_id();
            return seal::MemoryManager::GetPool();
        }

    private:
        std::vector<std::thread> threads;
        Channel<Task> chan;
    };


}