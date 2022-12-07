
#include "manager_workstream.hpp"

void services::WorkStream::add_task_to_write(std::unique_ptr<distribicom::WorkerTaskPart> &&tsk) {
    mtx.lock();
    to_write.emplace(std::move(tsk));
    mtx.unlock();
}

void services::WorkStream::write_next() {
    mtx.lock();
    if (!to_write.empty()) {
        StartWrite(to_write.front().get());
    }
    mtx.unlock();
}

void services::WorkStream::OnWriteDone(bool ok) {
    if (!ok) {
        std::cout << "Bad write" << std::endl;
    }
    // probably should ? only write when you have many things to write.
    mtx.lock();
    to_write.pop();
    mtx.unlock();

    write_next();
}
