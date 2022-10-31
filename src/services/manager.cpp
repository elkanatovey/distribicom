#include "manager.hpp"

namespace services {
    ::grpc::Status
    Manager::RegisterAsWorker(::grpc::ServerContext *context,
                              const ::distribicom::WorkerRegistryRequest *request,
                              ::distribicom::Ack *response) {
        return Service::RegisterAsWorker(context, request, response);
    }

    ::grpc::Status Manager::ReturnLocalWork(::grpc::ServerContext *context,
                                            ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                                            ::distribicom::Ack *response) {
        return Service::ReturnLocalWork(context, reader, response);
    }
}