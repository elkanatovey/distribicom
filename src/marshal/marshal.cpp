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

}
