#include "waitgroup.hpp"

namespace concurrency {
    void WaitGroup::add(int delta) {
        {
            count.fetch_add(delta); // TODO: understand order of operation parameter in these funcs.
        }
    }


    void WaitGroup::done() {
        {
            auto prev_val = count.fetch_add(-1);
            if (prev_val == 1) {
                cv.notify_all();
                return;
            }
            if (prev_val <= 0) {
                throw std::runtime_error("negative counter in wait group!");
            }
        }
    }

    void WaitGroup::wait() {
        {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [this] { return count.load() == 0; });
        }
    }
}