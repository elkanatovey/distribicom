#pragma once

#include <mutex>
#include <condition_variable>

namespace concurrency {

    class Counter {

    private:
        std::mutex m;
        std::condition_variable cv;
        int count = 0;
    public:
        Counter() : m(), cv() {}

        // not allowing move or copy.
        Counter(const Counter &) = delete;

        Counter(Counter &&) = delete;

        void add(int delta);

        void wait_for(int c);
    };

}