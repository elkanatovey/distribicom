#pragma once

#include "pir.hpp"
#include "pir_server.hpp"

namespace multiplication_utils {
    class EvaluatorWrapper {
        std::shared_ptr<seal::Evaluator> evaluator_;
        seal::EncryptionParameters enc_params_; // SEAL parameters
        seal::SEALContext context_ = seal::SEALContext(seal::EncryptionParameters());
        seal::Ciphertext trivial_zero;


        EvaluatorWrapper(std::shared_ptr<seal::Evaluator> evaluator,
                         seal::EncryptionParameters enc_params) : evaluator_(evaluator), enc_params_(enc_params) {
            context_ = seal::SEALContext(enc_params, true);
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
         * Assumes that we are in correct NTT form:
         *  ctx -> both not in ntt.
         *  ptx ->  either both are NTT or both aren't NTT.
         */
        void mult(const seal::Ciphertext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef DEBUG
            assert(!a.is_ntt_form() && !b.is_ntt_form())
#endif

            evaluator_->multiply(a, b, c);
        }

        // TODO: Note that plaintext type would be changed into a "modified-ptx" that can be split into two items.
        void mult_reg(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef DEBUG
            assert(a.is_ntt_form() && b.is_ntt_form())
#endif

            evaluator_->multiply_plain(b, a, c);
        }

        void mult_reg(const seal::Ciphertext &a, const seal::Plaintext &b, seal::Ciphertext &c) const {
            mult_reg(b, a, c);
        }

        // TODO: very in-efficient due to the plain-text type.
        void mult_modified(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef DEBUG
            assert(!a.is_ntt_form() && b.is_ntt_form())
#endif
            seal::Plaintext a1(enc_params_.plain_modulus().value());
            seal::Plaintext a2(enc_params_.plain_modulus().value());


            auto threshold = (enc_params_.plain_modulus().value() + 1) >> 1;

            auto coeff_count = a.coeff_count();
            for (int current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
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

            evaluator_->multiply_plain(b, a1, c);
            seal::Ciphertext tmp;
            evaluator_->multiply_plain(b, a1, tmp);

            evaluator_->add(c, tmp, c);
        }

        void mult_modified(const seal::Ciphertext &a, const seal::Plaintext &b, seal::Ciphertext &c) const {
            mult_modified(b, a, c);
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
            for (int current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
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