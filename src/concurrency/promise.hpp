
#pragma once
#include <memory>
#include "safelatch.h"
namespace concurrency {

    template<typename T>
    class promise {

    private:
        std::atomic<bool> done;
        std::shared_ptr<safelatch> wg;
        std::shared_ptr<T> value;

    public:
        promise(int n, std::shared_ptr<T> &result_store) : value(result_store) {
            wg = std::make_shared<safelatch>(n);
        }

        promise(int n, std::shared_ptr<T> &&result_store) : value(std::move(result_store)) {
            wg = std::make_shared<safelatch>(n);
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
            if (wg->done_waiting()) {
                throw std::runtime_error("promise set after it was done");
            }
            value = std::move(val);
        }

        std::shared_ptr<safelatch> get_latch() {
            return wg;
        }

        inline void count_down() {
            wg->count_down();
        }
    };


}
