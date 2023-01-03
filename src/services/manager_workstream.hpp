#include <grpc++/grpc++.h>
#include "distribicom.pb.h"
#include "queue"
#include "mutex"


namespace services {
    class WorkStream : public grpc::ServerWriteReactor<distribicom::WorkerTaskPart> {
        std::mutex mtx;
        std::queue<distribicom::WorkerTaskPart*> to_write;
        bool mid_write = false; // ensures that sequential calls to write would not write the same item twice.

        void OnDone() override {
            // todo: mark that this stream is closing, and no-one should hold onto it anymore.
            delete this;
        }

        void OnWriteDone(bool ok) override;

    public:

        void close() { Finish(grpc::Status::OK); }

        void add_task_to_write(distribicom::WorkerTaskPart* tsk);

        void write_next();


    };
}
