#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"

class Worker final : public distribicom::Worker::Service {
    ::grpc::Status SendTasks(::grpc::ServerContext *context, ::grpc::ServerReader<::distribicom::MatrixPart> *reader,
                             ::distribicom::Ack *response) override {
        // TODO: secure this stream.
        distribicom::MatrixPart tmp;
        while (reader->Read(&tmp)) {
            std::cout << "We are live." << std::endl;
        }
    }

};

