#pragma once

#include "pir.hpp"
#include "pir_server.hpp"


namespace multiplication_utils {
    /***
     * SplitPlaintext represents a plaintext that was intentionally split into two plaintexts such that their
     * sum equals the original.
     * using this type in ptx-ctx multiplication ensures associativity and commutativity hold.
     */
    typedef std::vector<seal::Plaintext> SplitPlaintext;

/***
 * This class wraps and modifies the behaviour of seal::Evaluator to add wanted multiplication for distribicom.
 */
    class EvaluatorWrapper {
        std::shared_ptr<seal::Evaluator> evaluator_;
        seal::EncryptionParameters enc_params_; // SEAL parameters
        seal::SEALContext context_ = seal::SEALContext(seal::EncryptionParameters());
        seal::Ciphertext trivial_zero;


        EvaluatorWrapper(std::shared_ptr<seal::Evaluator> evaluator,
                         seal::EncryptionParameters enc_params) : evaluator_(evaluator), enc_params_(enc_params) {
            context_ = seal::SEALContext(enc_params, true);

            trivial_zero = seal::Ciphertext(context_);
            trivial_zero.resize(2);
        }


    public:
        /***
         * Creates and returns a an initialized matrix_multiplier
         * @param evaluator
         * @param enc_params
         * @return a matrix multiplier
         */
        static std::shared_ptr<EvaluatorWrapper> Create(std::shared_ptr<seal::Evaluator> evaluator,
                                                        seal::EncryptionParameters enc_params) {

            auto eval = EvaluatorWrapper(evaluator, enc_params);
            return std::make_shared<EvaluatorWrapper>(std::move(eval));
        };

        /***
         * multiplies two ciphertexts using standard ciphertext multiplication
         * Assumes that we are in correct NTT form:
         *  ctx -> both not in ntt.
         *  ptx ->  either both are NTT or both aren't NTT.
         */
        void mult(const seal::Ciphertext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef MY_DEBUG
            assert(!a.is_ntt_form() && !b.is_ntt_form());
#endif

            evaluator_->multiply(a, b, c);
        }

        // TODO: Note that plaintext type would be changed into a "modified-ptx" that can be split into two items.
        /***
         * multiplies plaintext with a ciphertext using regular seal evaluator.
         * @param a
         * @param b
         * @param c
         */
        void mult_reg(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef MY_DEBUG
            assert(a.is_ntt_form());
            assert(b.is_ntt_form());
#endif

            evaluator_->multiply_plain(b, a, c);
        }

        // TODO: very in-efficient due to the plain-text type.
        /***
         * splits plaintext into two, then mults with ciphertext and then adds the sub results together.
         * should achieve associative/commutative ptx-ciphertext multiplication.
         * @param a
         * @param b
         * @param c
         */
        void mult_modified(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef MY_DEBUG
            assert(!a.is_ntt_form());
            assert(b.is_ntt_form());
#endif
            auto split_ptx = split_plaintext(a);


            evaluator_->multiply_plain(b, split_ptx[0], c);

            seal::Ciphertext tmp;
            evaluator_->multiply_plain(b, split_ptx[1], tmp);

            evaluator_->add(c, tmp, c);
        }

        SplitPlaintext split_plaintext(const seal::Plaintext &a) const {
#ifdef MY_DEBUG
            assert(!a.is_ntt_form());
#endif
            seal::Plaintext a1(enc_params_.poly_modulus_degree());
            seal::Plaintext a2(enc_params_.poly_modulus_degree());

            auto threshold = (enc_params_.plain_modulus().value() + 1) >> 1;

            auto coeff_count = a.coeff_count();
            for (uint32_t current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
                // plain modulus is odd, thus threshold is an even number.
                if (a[current_coeff] >= threshold) {
                    // skipping if to perform + 1 in case a[current_coeff] is odd.
                    a1[current_coeff] = (a[current_coeff] >> 1) + (a[current_coeff] & 1);
                    a2[current_coeff] = (a[current_coeff] >> 1);
                }
                else {
                    a1[current_coeff] = a[current_coeff];
                    a2[current_coeff] = 0;
                }
            }

            evaluator_->transform_to_ntt_inplace(a1, context_.first_parms_id());
            evaluator_->transform_to_ntt_inplace(a2, context_.first_parms_id());

            return {
                    std::move(a1),
                    std::move(a2),
            };
        }

        /***
         * multiplies plaintext with a ciphertext by encoding the plaintext into a ciphertext and then doing
         * ciphertext-ciphertext standard multiplication.
         * @param a
         * @param b
         * @param c
         */
        void mult_slow(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
            evaluator_->add_plain(trivial_zero, a, c);
            evaluator_->multiply_inplace(c, b);
        }

        /***
        * take vector along with constant and do result+= vector*constant
        * @param right vector from above
        * @param left const
        * @param result place to add to
        ***/
        void multiply_add(const std::uint64_t left, const seal::Plaintext &right, seal::Plaintext &result) const {
            if (left == 0) {
                return;
            }

            auto ptx_mod = enc_params_.plain_modulus().value();

            auto coeff_count = right.coeff_count();
            for (std::uint64_t current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
                result[current_coeff] += (left * right[current_coeff]);
                if (result[current_coeff] >= ptx_mod) {
                    result[current_coeff] -= ptx_mod;
                }
            }
        }

        void multiply_add(const std::uint64_t left, const seal::Ciphertext &right, seal::Ciphertext &result) const {
            if (left == 0) {
                return;
            }
            evaluator_->add_inplace(result, right);
        }
    };
}