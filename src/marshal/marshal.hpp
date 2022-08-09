#pragma once

#include "pir.hpp"
#include "pir_server.hpp"
#include "vector"


class Marshaller {
    seal::EncryptionParameters enc_params; // SEAL parameters
    seal::SEALContext context;

public:
    Marshaller(seal::EncryptionParameters enc_params);

    vector<seal::Plaintext> unmarshal_plaintext_vector(vector<vector<std::byte>> ptxs);

    template<typename T>
    void unmarshal_seal_object(std::vector<seal::seal_byte> in, T &out) {
        out.load(context, &in[0], in.size());
    }
};

