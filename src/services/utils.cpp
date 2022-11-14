#include "utils.hpp"

namespace services::utils{
    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                   const services::constants::metadata &md) {
        auto ntmp = mp.find(std::string(md));
        return std::stoi(std::string(ntmp->second.data(), ntmp->second.size()));
    }

    void add_metadata_size(grpc::ClientContext &context, const services::constants::metadata &md, int size) {
        context.AddMetadata(std::string(md), std::to_string(size));
    }

    std::string extract_ipv4(grpc::ServerContext *pContext) {
        auto fullip = std::string(pContext->peer());
        const std::string delimiter("ipv4:");
        fullip.erase(0, fullip.find(delimiter) + delimiter.length());
        return fullip.substr(0, fullip.find(':'));
    }

    seal::EncryptionParameters setup_enc_params(const distribicom::AppConfigs &cnfgs) {
        seal::EncryptionParameters enc_params(seal::scheme_type::bgv);
        auto N = cnfgs.configs().polynomial_degree();
        auto logt = cnfgs.configs().logarithm_plaintext_coefficient();

        enc_params.set_poly_modulus_degree(N);
        // TODO: Use correct BGV parameters.
        enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
        enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));
        return enc_params;
    }
}

