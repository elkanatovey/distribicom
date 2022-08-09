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
            in.save(&out[0], out.size());
        }

        template<typename T>
        void marshal_seal_vector(const T &in, marshaled_seal_object &out) {
            in.save(&out[0], out.size());
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


        template<typename T>
        void prepare_seal_marshal_object(marshaled_seal_object &in, const T &seal_object) {
            in.resize(seal_object.save_size(context));
        }

        template<typename T>
        void prepare_seal_marshal_object(std::vector<marshaled_seal_object> &in, const std::vector<T> &seal_object) {
            for (std::uint64_t i = 0; i < in.size(); ++i) {
                prepare_seal_marshal_object(in[i], seal_object[i]);
            }
        }


    };

}