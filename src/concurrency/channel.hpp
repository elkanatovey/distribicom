#pragma once


#include <queue>
#include <mutex>
#include <condition_variable>

namespace concurrency {

    template<class T>
    struct Result {
        T answer;
        bool ok;
    };


    template<class T>
    class Channel {
    private:

        bool closed = false;
        std::queue<T> q;
        mutable std::mutex m;
        std::condition_variable c;

        /**
         * USAGE: should be called when channel resources are locked!
         * the proper way the channel outputs anything, will always attempt to drain the channel before stating it is closed
         * @return
         */
        inline Result<T> pop_chan() {
            if (!q.empty()) {
                Result<T> out{std::move(q.front()), true};
                q.pop();
                return out;
            }
            if (closed) {
                return Result<T>{T(), false};
            } // result is not OK.

            throw std::runtime_error("Channel::pop_chan() - unexpected state.");
        }

    public:
        Channel() : q(), m(), c() {}

        ~Channel() {
            close();
        }

        // not allowing copy or moving of a channel.
        Channel(const Channel &) = delete;

        Channel(Channel &&) = delete;


        /**
         * Adds an element to the queue.
         */
        void write(T &&t) {
            std::lock_guard<std::mutex> lock(m);
            if (closed) {
                throw std::runtime_error("Channel::write() - channel closed.");
            }
            q.push(std::move(t));
            c.notify_one();
        }

        /**
         * Get the "front"-element.
         * If there is nothing to read from the channel, wait till an element was written on another thread.
         */
        Result<T> read() {
            std::unique_lock<std::mutex> lock(m);
            // returns if q is not empty, and if channel is not closed. // todo verify this expression.
            c.wait(lock, [&] { return (!q.empty() || closed); });

            return pop_chan();
        }

        /**
         * Closes the channel, anyone attempting to read from a closed channel should quickly receive read failure.
         */
        void close() {
            std::lock_guard<std::mutex> lock(m);
            closed = true;
            c.notify_all();
        }

    };
}