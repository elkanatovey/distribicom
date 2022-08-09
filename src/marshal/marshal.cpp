//
// Created by Jonathan Weiss on 09/08/2022.
//

#include "marshal.hpp"


Marshaller::Marshaller(seal::EncryptionParameters enc_params) : enc_params(enc_params), context(enc_params, true) {}


