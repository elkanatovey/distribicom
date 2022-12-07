#pragma once

#include "constants.hpp"
#include <grpc++/grpc++.h>
#include <seal/seal.h>
#include "distribicom.grpc.pb.h"


namespace services::utils {
    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                   const services::constants::metadata &md);

    void add_metadata_size(grpc::ClientContext &context, const services::constants::metadata &md, int size);

    void
    add_metadata_string(grpc::ClientContext &context, const services::constants::metadata &md, const std::string &str);

    std::string extract_string_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                             const services::constants::metadata &md);

    std::string extract_ip(grpc::ServerContext *pContext);

    std::string extract_ip(std::string peer);

    seal::EncryptionParameters setup_enc_params(const distribicom::AppConfigs &cnfgs);

    std::string byte_vec_to_64base_string(const std::vector<std::byte> &bytes);
}


