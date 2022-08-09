#pragma once

#include "seal/seal.h"

namespace marshal {
    typedef std::vector<seal::seal_byte> marshaled_seal_object;

    class Marshaller {
        seal::EncryptionParameters enc_params; // SEAL parameters
        seal::SEALContext context;

        explicit Marshaller(seal::EncryptionParameters enc_params);

    public:

        static std::shared_ptr<Marshaller> Create(seal::EncryptionParameters enc_params);

        template<typename T>
        void marshal_seal_object(const T &in, marshaled_seal_object &out) {
            out.resize(in.save_size(context));
            in.save(&out[0], out.size());
        }

        template<typename T>
        void marshal_seal_vector(const std::vector<T> &in, std::vector<marshaled_seal_object> &out) {
            for (std::uint64_t i = 0; i < in.size(); ++i) {
                marshal_seal_object(in[i], out[i]);
            }
        }

        template<typename T>
        void unmarshal_seal_vector(const std::vector<marshaled_seal_object> &in, std::vector<T> &out) {
            for (std::uint64_t i = 0; i < in.size(); i++) {
                unmarshal_seal_object(in[i], out[i]);
            }
        }


        template<typename T>
        void unmarshal_seal_object(const marshaled_seal_object &in, T &out) {
            out.load(context, &in[0], in.size());
        }

    };

}