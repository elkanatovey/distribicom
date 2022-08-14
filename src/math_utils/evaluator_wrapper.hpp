#pragma once

#include "seal/seal.h"
#include <cassert>
#include "defines.h"

namespace multiplication_utils {
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


    public:
        seal::EncryptionParameters enc_params; // SEAL parameters
        seal::SEALContext context;
        std::unique_ptr<seal::Evaluator> evaluator;

        /***
         * Creates and returns a an initialized matrix_multiplier
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
        void mult_modified(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

        /***
         * mult_modified receives a split ptx and multiplies it with the ciphertext.
         * all params are in ntt forms.
         */
        void mult_modified(const SplitPlaintextNTTForm &a, const seal::Ciphertext &b, seal::Ciphertext &c) const;

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

        void add_plain(const seal::Plaintext &a, const seal::Plaintext &b, seal::Plaintext &c);


        /***
        * take vector along with constant and do sum += vector*constant.
        ***/
        void multiply_add(const std::uint64_t left, const seal::Plaintext &right, seal::Plaintext &sum) const;

        void multiply_add(const std::uint64_t left, const seal::Ciphertext &right, seal::Ciphertext &sum) const;

        void trivial_ciphertext(const seal::Plaintext &ptx, seal::Ciphertext &result);
    };
}