#pragma once

#include <queue>
#include <condition_variable>

namespace concurrency {
    class WaitGroup {
    private:
        std::mutex m;
        std::condition_variable cv;
        std::atomic_int32_t count = 0;
    public:
        WaitGroup() : m(), cv() {}

        // not allowing move or copy.
        WaitGroup(const WaitGroup &) = delete;

        WaitGroup(WaitGroup &&) = delete;

        void add(int delta);

        void done();

        void wait();
    };
}