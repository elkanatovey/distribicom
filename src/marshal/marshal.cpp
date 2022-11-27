//
// Created by Jonathan Weiss on 09/08/2022.
//

#include "marshal.hpp"

namespace marshal{
    Marshaller::Marshaller(seal::EncryptionParameters enc_params) : enc_params(enc_params), context(enc_params, true) {}

    std::shared_ptr<Marshaller> Marshaller::Create(seal::EncryptionParameters enc_params) {
        auto marsh = Marshaller(enc_params);
        return std::make_shared<Marshaller>(std::move(marsh));
    }

    void Marshaller::marshal_query_vector(const std::vector<std::vector<seal::Ciphertext>> &in,
                                          distribicom::ClientQueryRequest &out) const {
        for (std::uint64_t i = 0; i < in[0].size(); ++i) {
            auto marshaled_ctx = marshal_seal_object(in[0][i]);
            distribicom::Ciphertext* current_ctx = out.add_query_dim1();
            current_ctx->set_data(marshaled_ctx);
        }
        if(in.size()>1) {
            for (std::uint64_t i = 0; i < in[1].size(); ++i) {
                auto marshaled_ctx = marshal_seal_object(in[1][i]);
                distribicom::Ciphertext *current_ctx = out.add_query_dim2();
                current_ctx->set_data(marshaled_ctx);
            }
        }
    }

    std::vector<std::vector<seal::Ciphertext>>
    Marshaller::unmarshal_query_vector(const distribicom::ClientQueryRequest &in) const {
        std::vector<seal::Ciphertext> query_dim1;
        query_dim1.reserve((in.query_dim1_size()));

        for(std::uint64_t i = 0; i < in.query_dim1_size(); i++) {
            auto ctx = unmarshal_seal_object<seal::Ciphertext>(in.query_dim1(i).data());
            query_dim1.push_back(std::move(ctx));
        }

        std::vector<seal::Ciphertext> query_dim2;
        query_dim2.reserve((in.query_dim2_size()));

        for(std::uint64_t j = 0; j < in.query_dim2_size(); j++) {
            auto ctx = unmarshal_seal_object<seal::Ciphertext>(in.query_dim2(j).data());
            query_dim2.push_back(std::move(ctx));
        }

        std::vector<std::vector<seal::Ciphertext>> query;
        query.push_back(query_dim1);
        if(!query_dim2.empty()){query.push_back(query_dim2);}

        return query;
    }

}
