#include "safelatch.h"
namespace concurrency {
    void safelatch::count_down() {
        auto prev = safety.fetch_add(-1);
        if (prev <= 0) {
            throw std::runtime_error(
                "count_down:: latch's value is less than 0, this is a bug that can lead to deadlock!");
        }
    }

    bool safelatch::done_waiting() {
        return safety.load() == 0;
    }
}