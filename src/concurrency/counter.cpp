
#include "counter.hpp"

void concurrency::Counter::wait_for(int c) {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this, c] { return count >= c; });
}

void concurrency::Counter::add(int delta) {
    std::lock_guard<std::mutex> lock(m);
    count += delta;
    cv.notify_all();
}
