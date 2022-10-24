
#pragma once

#include "distribicom.pb.h"
#include "distribicom.grpc.pb.h"
#include "manager.hpp"

namespace services {
    // uses both the Manager and the Server services to complete a full distribicom server.
    // TODO: These are not interfaces, find a way to use them correctly, currently i have a diamond inheritance!!!
    class FullServer : public virtual distribicom::Server::Service, virtual distribicom::Manager::Service {

        // using composition to implement the interface of the manager.
        services::Manager manager;

    };
}
