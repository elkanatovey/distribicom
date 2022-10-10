#pragma once

class channel {

};


#include <queue>
#include <mutex>
#include <condition_variable>

template<class T>
struct Result {
    T answer;
    bool ok;
};

template<class T>
class Channel {
public:
    Channel() : q(), m(), c() {}

    ~Channel() {
        close();
    }

    // Add an element to the queue.
    void write(T t) {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    // Get the "front"-element.
    // If the queue is empty, wait till an element is available.
    Result<T> read() {
        std::unique_lock<std::mutex> lock(m);
        // returns if q is not empty, and if channel is not closed. // todo verify this expression.
        c.wait(lock, [&] { return (!q.empty() || closed); });
        if (closed) { return Result<T>{T(), false}; } // result is not OK.
        T val = q.front();
        q.pop();
        return Result<T>{val, true};
    }

    void close() {
        std::lock_guard<std::mutex> lock(m);
        closed = true;
        c.notify_all();
    }


private:
    bool closed = false;
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};