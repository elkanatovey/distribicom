
#include "manager_workstream.hpp"

void services::WorkStream::add_task_to_write(distribicom::WorkerTaskPart* tsk) {
    mtx.lock();
    to_write.emplace(tsk);
    mtx.unlock();
}

void services::WorkStream::write_next() {
    mtx.lock();
    if (!mid_write && !to_write.empty()) {
        mid_write = true;
        StartWrite(to_write.front());
    }
    mtx.unlock();
}

void services::WorkStream::OnWriteDone(bool ok) {

    if (!ok) {
        std::cout << "Bad write" << std::endl;
    }

    mtx.lock();
    mid_write = false;
    if (!to_write.empty()) { to_write.pop(); }
    mtx.unlock();

    write_next();
}
