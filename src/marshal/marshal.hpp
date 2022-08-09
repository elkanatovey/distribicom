#pragma once

#include "pir.hpp"
#include "pir_server.hpp"
#include "vector"

namespace marshal{
    typedef std::vector<seal::seal_byte> marshaled_seal_object;

    class Marshaller {
        seal::EncryptionParameters enc_params; // SEAL parameters
        seal::SEALContext context;
        Marshaller(seal::EncryptionParameters enc_params);
    public:


        static std::shared_ptr<Marshaller> Create(seal::EncryptionParameters enc_params);

        template<typename T>
        void unmarshal_seal_vector(std::vector<marshaled_seal_object> in, std::vector<T> &out) {
            for (uint64_t i=0; i<in.size(); i++){
                unmarshal_seal_object(in[i], out[i]);
            }
        }


        template<typename T>
        void unmarshal_seal_object(marshaled_seal_object in, T &out) {
            out.load(context, &in[0], in.size());
        }
    };

}