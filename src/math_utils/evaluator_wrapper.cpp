#include "evaluator_wrapper.hpp"

namespace math_utils {

    void EvaluatorWrapper::multiply_add(const std::uint64_t left, const seal::Plaintext &right,
                                        seal::Plaintext &sum) const {
        if (left == 0) {
            return;
        }

        auto ptx_mod = enc_params.plain_modulus().value();

        auto coeff_count = right.coeff_count();
        for (std::uint64_t current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
            sum[current_coeff] += (left * right[current_coeff]);
            if (sum[current_coeff] >= ptx_mod) {
                sum[current_coeff] -= ptx_mod;
            }
        }
    }

    void EvaluatorWrapper::multiply_add(const std::uint64_t left, const seal::Ciphertext &right,
                                        seal::Ciphertext &sum) const {
        if (left == 0) {
            return;
        }
        evaluator->add_inplace(sum, right);
    }

    void EvaluatorWrapper::add_plain(const seal::Plaintext &a, const seal::Plaintext &b,
                                     seal::Plaintext &c) {
        seal::Plaintext tmp;
        tmp = a;

        add_plain_in_place(tmp, b);
        c = tmp;
    }

    void EvaluatorWrapper::add_plain_trivial(const seal::Plaintext &a, const seal::Plaintext &b,
                                             seal::Ciphertext &out) const {
        seal::Ciphertext tmp_a, tmp_b;
        trivial_ciphertext(a, tmp_a);
        trivial_ciphertext(b, tmp_b);
        evaluator->add(tmp_a, tmp_b, out);
    }


    void EvaluatorWrapper::add_plain_in_place(seal::Plaintext &a, const seal::Plaintext &b) const {
        auto ptx_mod = enc_params.plain_modulus().value();

        auto coeff_count = a.coeff_count();
        for (std::uint64_t current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
            a[current_coeff] += b[current_coeff];
            if (a[current_coeff] >= ptx_mod) {
                a[current_coeff] -= ptx_mod;
            }
        }
    }

    void EvaluatorWrapper::mult_slow(const seal::Plaintext &a, const seal::Ciphertext &b,
                                     seal::Ciphertext &c) const {
        evaluator->add_plain(trivial_zero, a, c);
        evaluator->multiply_inplace(c, b);
    }

    /**
     * The resulted SplitPlaintextNTTForm is in NTT.
     */
    SplitPlaintextNTTForm EvaluatorWrapper::split_plaintext(const seal::Plaintext &a) const {
#ifdef DISTRIBICOM_DEBUG
        assert(!a.is_ntt_form());
#endif

        seal::Plaintext a1(enc_params.poly_modulus_degree());
        seal::Plaintext a2(enc_params.poly_modulus_degree());

        auto threshold = (enc_params.plain_modulus().value() + 1) >> 1;

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

#ifdef DISTRIBICOM_DEBUG
        assert_plain_is_under_threshold(a1);
        assert_plain_is_under_threshold(a2);
#endif

        evaluator->transform_to_ntt_inplace(a1, context.first_parms_id());
        evaluator->transform_to_ntt_inplace(a2, context.first_parms_id());

        return {
                std::move(a1),
                std::move(a2),
        };
    }

    void
    EvaluatorWrapper::mult_modified(const SplitPlaintextNTTForm &a, const seal::Ciphertext &b,
                                    seal::Ciphertext &c) const {
#ifdef DISTRIBICOM_DEBUG
        assert(2 == a.size());
        for (auto &a_i: a) {
            assert(a_i.is_ntt_form());
        }
        assert(b.is_ntt_form());
#endif
        evaluator->multiply_plain(b, a[0], c);

        seal::Ciphertext tmp;
        evaluator->multiply_plain(b, a[1], tmp);

        evaluator->add(c, tmp, c);
    }

    void
    EvaluatorWrapper::mult_modified(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef DISTRIBICOM_DEBUG
        assert(!a.is_ntt_form());
        assert(b.is_ntt_form());
#endif
        auto split_ptx = split_plaintext(a);
        mult_modified(split_ptx, b, c);
    }

    void EvaluatorWrapper::mult_reg(const seal::Plaintext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef DISTRIBICOM_DEBUG
        assert(a.is_ntt_form());
        assert(b.is_ntt_form());
#endif

        evaluator->multiply_plain(b, a, c);
    }

    void
    EvaluatorWrapper::mult(const seal::Ciphertext &a, const seal::Ciphertext &b, seal::Ciphertext &c) const {
#ifdef DISTRIBICOM_DEBUG
        assert(!a.is_ntt_form());
        assert(!b.is_ntt_form());
#endif

        evaluator->multiply(a, b, c);
    }

    std::shared_ptr<EvaluatorWrapper> EvaluatorWrapper::Create(seal::EncryptionParameters enc_params) {
        auto eval = EvaluatorWrapper(enc_params);
        return std::make_shared<EvaluatorWrapper>(std::move(eval));
    }

    EvaluatorWrapper::EvaluatorWrapper(seal::EncryptionParameters enc_params) :
            enc_params(enc_params),
            context(enc_params, true) {

        evaluator = std::make_unique<seal::Evaluator>(context);
        trivial_zero = seal::Ciphertext(context);
        trivial_zero.resize(2);
    }

    void EvaluatorWrapper::trivial_ciphertext(const seal::Plaintext &ptx, seal::Ciphertext &result) const {
        evaluator->add_plain(trivial_zero, ptx, result);
    }

    void EvaluatorWrapper::assert_plain_is_under_threshold(const seal::Plaintext &plaintext) const {
        auto threshold = (enc_params.plain_modulus().value() + 1) >> 1;
        auto coeff_count = plaintext.coeff_count();
        for (uint32_t current_coeff = 0; current_coeff < coeff_count; current_coeff++) {
            assert(plaintext[current_coeff] < threshold);
        }
    }
}
