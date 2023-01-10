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

        static seal::MemoryPoolHandle GetSealPoolHandle();

    private:
        std::vector<std::thread> threads;
        Channel<Task> chan;
    };


}