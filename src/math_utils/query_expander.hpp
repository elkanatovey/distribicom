#pragma once

#include <seal/seal.h>
#include <vector>

namespace math_utils {
    /***
     * Represents a query in a specific dimension. (unlike sealPIR where each query contains multiple dimensions).
     */
    typedef std::vector<seal::Ciphertext> Query;

    /***
     * expand_query takes a part of multi-dimensional query and expand one part of.
     */
    class QueryExpander {
    public:
        static std::shared_ptr<QueryExpander> Create(const seal::EncryptionParameters enc_params);

        std::vector<seal::Ciphertext> expand_query(std::vector<seal::Ciphertext> query_i, uint64_t n_i, seal::GaloisKeys &galkey);

        // TODO: how to make it public in tests only? only? not public?
        std::vector<seal::Ciphertext> __expand_query(const seal::Ciphertext &encrypted,
                                                     uint32_t m, seal::GaloisKeys &galkey);

    private:
        explicit QueryExpander(const seal::EncryptionParameters enc_params);

        seal::EncryptionParameters enc_params_;
        std::shared_ptr<seal::Evaluator> evaluator_;

        void multiply_power_of_X(const seal::Ciphertext &encrypted, seal::Ciphertext &destination, uint32_t index);
    };
}