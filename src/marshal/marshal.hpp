#pragma once

#include "seal/seal.h"
#include "distribicom.grpc.pb.h"

namespace marshal {
    typedef std::vector<seal::seal_byte> marshaled_seal_object;

    class Marshaller {
        seal::EncryptionParameters enc_params; // SEAL parameters
        seal::SEALContext context;

        explicit Marshaller(seal::EncryptionParameters enc_params);

    public:

        static std::shared_ptr<Marshaller> Create(seal::EncryptionParameters enc_params);

        template<typename T>
        void marshal_seal_object(const T &in, marshaled_seal_object &out) const {
            out.resize(in.save_size());
            in.save(&out[0], out.size());
        }

        template<typename T>
        std::string marshal_seal_object(const T &in) const {
            marshaled_seal_object out;
            out.resize(in.save_size());
            in.save(&out[0], out.size());
            return {(char *) &out[0], out.size()};
        }

        template<typename T>
        void marshal_seal_vector(const std::vector<T> &in, std::vector<marshaled_seal_object> &out) const {
            for (std::uint64_t i = 0; i < in.size(); ++i) {
                marshal_seal_object(in[i], out[i]);
            }
        }

        /**
         * vector of vectors serialisation into protobuf
         */
        void marshal_query_vector(const std::vector<std::vector<seal::Ciphertext>> &in, distribicom::ClientQueryRequest &out)
        const;

        /**
        * vector of vectors deserialisation from protobuf
        */
        std::vector<std::vector<seal::Ciphertext>> unmarshal_query_vector(const distribicom::ClientQueryRequest &in)
        const;

        template<typename T>
        void unmarshal_seal_vector(const std::vector<marshaled_seal_object> &in, std::vector<T> &out) const {
            for (std::uint64_t i = 0; i < in.size(); i++) {
                unmarshal_seal_object(in[i], out[i]);
            }
        }


        template<typename T>
        void unmarshal_seal_object(const marshaled_seal_object &in, T &out) const {
            out.load(context, &in[0], in.size());
        }

        template<typename T>
        T unmarshal_seal_object(const std::string &in) const {
            T t;
            t.load(context, (seal::seal_byte *) &in[0], in.size());
            return t;
        }
    };

}