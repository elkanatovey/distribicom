#include <grpc++/grpc++.h>
#include "distribicom.pb.h"
#include "queue"
#include "mutex"


namespace services {
    class WorkStream : public grpc::ServerWriteReactor<distribicom::WorkerTaskPart> {
        std::mutex mtx;
        std::queue<std::unique_ptr<distribicom::WorkerTaskPart>> to_write;
    public:
        void OnDone() override {
            // todo: mark that this stream is closing, and no-one should hold onto it anymore.
            delete this;
        }

        void add_task_to_write(std::unique_ptr<distribicom::WorkerTaskPart> &&tsk);

        void write_next();

        // after each succsessful write, pops from the queue, then attempts to do the next write.
        void OnWriteDone(bool ok) override;
    };
}
