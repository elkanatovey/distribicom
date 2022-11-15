#include "client_service.hpp"

namespace services {
    ClientListener::ClientListener(distribicom::AppConfigs &_app_configs): app_configs(_app_configs), mrshl(){
        seal::EncryptionParameters enc_params = setup_enc_params();
        PirParams p;
        gen_pir_params(app_configs.configs().number_of_elements(), app_configs.configs().size_per_element(),
                       app_configs.configs().dimensions(), enc_params, p,  app_configs.configs().use_symmetric(),
                       app_configs.configs().use_batching(), app_configs.configs().use_recursive_mod_switching());
        client = make_unique<PIRClient>(PIRClient(enc_params, std::move(p)));

    }

    grpc::Status ClientListener::Answer(grpc::ServerContext *context, const distribicom::Ciphertext *answer,
                                        distribicom::Ack *response) {
        return Service::Answer(context, answer, response);
    }

    grpc::Status
    ClientListener::TellNewRound(grpc::ServerContext *context, const distribicom::TellNewRoundRequest *request,
                                 distribicom::Ack *response) {
        return Service::TellNewRound(context, request, response);
    }

    seal::EncryptionParameters ClientListener::setup_enc_params() const {
        seal::EncryptionParameters enc_params(seal::scheme_type::bgv);
        auto N = app_configs.configs().polynomial_degree();
        auto logt = app_configs.configs().logarithm_plaintext_coefficient();

        enc_params.set_poly_modulus_degree(N);
        // TODO: Use correct BGV parameters.
        enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
        enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));
        return enc_params;
    }
} // services