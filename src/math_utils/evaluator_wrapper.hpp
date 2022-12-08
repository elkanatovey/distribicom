#pragma once

#include "seal/seal.h"
#include <cassert>
#include "defines.h"

namespace math_utils {
    /***
     * SplitPlaintextNTTForm represents a plaintext that was intentionally split into two plaintexts such that their
     * sum equals the original.
     * using this type in ptx-ctx multiplication ensures associativity and commutativity hold.
     */
    typedef std::vector<seal::Plaintext> SplitPlaintextNTTForm;

/***
 * This class wraps and modifies the behaviour of seal::Evaluator to add wanted multiplication for distribicom.
 */
    class EvaluatorWrapper {
        seal::Ciphertext trivial_zero;

        EvaluatorWrapper(seal::EncryptionParameters enc_params);

        void assert_plain_is_under_threshold(const seal::Plaintext &plaintext) const;

    public:
        seal::EncryptionParameters enc_params; // SEAL parameters
        seal::SEALContext context;
        std::unique_ptr<seal::Evaluator> evaluator;

        /***
         * Creates and returns a an initialized MatrixOperations
         * @param evaluator
         * @param enc_params
         * @return a matrix multiplier
         */
        static std::shared_ptr<EvaluatorWrapper> Create(seal::EncryptionParameters enc_params);;

        /***
         * multiplies two ciphertexts using standard ciphertext multiplication
         * Assumes that we are in correct NTT form:
         *  ctx -> both not in ntt.
         *  ptx ->  either both are NTT or both aren't NTT.
         */
        void mult(const seal::Ciphertext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

        inline void add(const seal::Ciphertext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
            evaluator->add(a, b, c);
        };

        // TODO: Note that plaintext type would be changed into a "modified-ptx" that can be split into two items.
        /***
         * multiplies plaintext with a ciphertext using regular seal evaluator.
         */
        void mult_reg(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

        // TODO: very in-efficient due to the plain-text type.
        /***
         * splits plaintext into two, then mults with ciphertext and then adds the sub results together.
         * should achieve associative/commutative ptx-ciphertext multiplication.
         */
        void mult(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

        inline void mult(const seal::Ciphertext &a, const seal::Plaintext &b, seal::Ciphertext &c) const {
            mult(b, a, c);
        }

        /***
         * mult receives a split ptx and multiplies it with the ciphertext.
         * all params are in ntt forms.
         */
        void mult(const SplitPlaintextNTTForm &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

        inline void mult(const seal::Ciphertext &a, const SplitPlaintextNTTForm &b, seal::Ciphertext &c) const {
            mult(b, a, c);
        }

        SplitPlaintextNTTForm split_plaintext(const seal::Plaintext &a) const;

        /***
         * multiplies plaintext with a ciphertext by encoding the plaintext into a ciphertext and then doing
         * ciphertext-ciphertext standard multiplication.
         * @param a
         * @param b
         * @param c
         */
        void mult_slow(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

        /***
         * adds the two plaintexts and adds into the left plaintext (a).
         */
        void add_plain_in_place(seal::Plaintext &a, const seal::Plaintext &b) const;

        void add_plain(const seal::Plaintext &a, const seal::Plaintext &b, seal::Plaintext &c) const;

        void add_plain_trivial(const seal::Plaintext &a, const seal::Plaintext &b,
                               seal::Ciphertext &out) const;


        void scalar_multiply(std::uint64_t scalar, const seal::Ciphertext &right, seal::Ciphertext &sum) const;

        void scalar_multiply(std::uint64_t scalar, const seal::Plaintext &ptx, seal::Ciphertext &sum) const;

        void trivial_ciphertext(const seal::Plaintext &ptx, seal::Ciphertext &result) const;

        /***
         * writes ptx embedding of ctx to ptx_decomposition after relinearizing ctx and putting switching to last modulus
         */
        void get_ptx_embedding(const seal::Ciphertext &ctx, const seal::RelinKeys& relin_keys, std::vector<seal::Plaintext>  &ptx_decomposition) const;

        void compose_to_ctx(const std::vector<seal::Plaintext> &ptx_decomposition, seal::Ciphertext &decoded) const;
    };
}