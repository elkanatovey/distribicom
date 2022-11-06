#include "manager.hpp"

namespace services {
    ::grpc::Status
    Manager::RegisterAsWorker(::grpc::ServerContext *context,
                              const ::distribicom::WorkerRegistryRequest *request,
                              ::distribicom::Ack *response) {

        auto requesting_worker = context->peer();
        // generate a request back ? to the same port?
        // maybe in the request i need to state the ports im listening to, and other such things.
        return Service::RegisterAsWorker(context, request, response);
    }

    ::grpc::Status Manager::ReturnLocalWork(::grpc::ServerContext *context,
                                            ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                                            ::distribicom::Ack *response) {
        return Service::ReturnLocalWork(context, reader, response);
    }
}