#pragma once

#include <future>
#include "pir_client.hpp"
#include "db.hpp"
#include "manager.hpp"
#include "distribicom.grpc.pb.h"
#include "distribicom.pb.h"

namespace services {
/**
* ClientInfo stores objects related to an individual client
*/
    struct ClientInfo {
        distribicom::ClientQueryRequest query_info_marshaled;
        distribicom::GaloisKeys galois_keys_marshaled;

        PirQuery query;
        seal::GaloisKeys galois_keys;
        std::unique_ptr<distribicom::Client::Stub> client_stub;
    };
    struct ClientDB {
        mutable std::shared_mutex mutex;
        std::map<uint32_t, std::unique_ptr<ClientInfo>> id_to_info;
        std::uint64_t client_counter = 0;
    };
}

