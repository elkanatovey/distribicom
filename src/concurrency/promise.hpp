
#pragma once

#include <latch>
#include <memory>
#include <atomic>

namespace concurrency {

    template<typename T>
    class promise {

    private:
        std::atomic<int> safety;
        std::atomic<bool> done;
        std::shared_ptr<std::latch> wg;
        std::shared_ptr<T> value;

    public:
        promise(int n, std::shared_ptr<T> &result_store) : safety(n), value(result_store) {
            wg = std::make_shared<std::latch>(n);
        }

        promise(int n, std::shared_ptr<T> &&result_store) : safety(n), value(std::move(result_store)) {
            wg = std::make_shared<std::latch>(n);
        }

        std::shared_ptr<T> get() {
            if (done.load()) {
                return value;
            }
            wg->wait();
            done.store(true);
            return value;
        }

        /***
         * Sets a new value iff the promise is not already done.
         * Notice, this should not be called after the latch reaches zero!
         */
        void set(std::shared_ptr<T> val) {
            if (done.load()) {
                return;
            }
            if (safety.load() == 0) {
                throw std::runtime_error("promise set after it was done");
            }
            value = std::move(val);
        }

        std::shared_ptr<std::latch> &get_latch() {
            return wg;
        }

        inline void count_down() {
            wg->count_down();
            auto prev = safety.fetch_add(-1);
            if (prev <= 0) {
                throw std::runtime_error(
                        "promise::count_down:: latch's value is less than 0, this is a bug that can lead to deadlock!");
            }
        }
    };


}
