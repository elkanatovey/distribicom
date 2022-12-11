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
                T val = q.front();
                q.pop();
                return Result<T>{val, true};
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
        void write(T t) {
            std::lock_guard<std::mutex> lock(m);
            if (closed) {
                throw std::runtime_error("Channel::write() - channel closed.");
            }
            q.push(t);
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
         * read_for behaves like read(), but ensures the caller does not block forever on the channel.
         * After the given duration the channel returns a result of read failure.
         */ // TODO: should return something that indicates timeout.
        template<typename _Rep, typename _Period>
        Result<T> read_for(const std::chrono::duration<_Rep, _Period> &dur) {
            auto max_timeout = std::chrono::steady_clock::now() + dur;

            std::unique_lock<std::mutex> lock(m);
            // returns if q is not empty, and if channel is not closed.
            c.wait_until(lock, max_timeout, [&] { return (!q.empty() || closed); });

            if (std::chrono::steady_clock::now() > max_timeout) {
                return Result<T>{T(), false}; // timeout passed..
            }

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