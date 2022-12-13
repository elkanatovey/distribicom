#pragma once

#include <latch>
#include <atomic>

namespace concurrency {

    class safelatch : public std::latch {
        std::atomic<int> safety;
    public:
        explicit safelatch(int count) : std::latch(count), safety(count) {};

        bool done_waiting();

        void count_down();

    };

}
