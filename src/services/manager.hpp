#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"

namespace services {
    // contains the workers and knows how to distribute their work.
    // shouls be able to give a Promise for a specific round and fullfill it once all workers have sent their jobs.
    // can use Frievalds to verify their work.

    class Manager : public distribicom::Manager::Service {
    private:
        distribicom::AppConfigs app_configs;

    public:
        // PartialWorkStream i save this on my server map.
        ::grpc::Status
        RegisterAsWorker(::grpc::ServerContext *context, const ::distribicom::WorkerRegistryRequest *request,
                         ::distribicom::Ack *response) override;

        // a worker should send its work, along with credentials of what it sent.
        ::grpc::Status
        ReturnLocalWork(::grpc::ServerContext *context, ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                        ::distribicom::Ack *response) override;

        // This one is the holder of the DB.


    };
}

