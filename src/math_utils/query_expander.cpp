// Extracted code from sealPIR, to specifically expanding single part of PIRQuery.

#include "query_expander.hpp"
#include "seal/seal.h"
#include <vector>
#include <cmath>
#include <stdexcept>

namespace multiplication_utils {

    /***
     * expand_query takes a part of multi-dimensional query and expand one part of.
     *  in sealPIR, this is consider a part out of a full query that should be expanded.
     * @param query_i the query_i to be expanded in dimension i
     * @param n_i - number of elements in dimension i
     * @return
     */
    void QueryExpander::expand_query(Query query_i, std::uint64_t n_i, seal::GaloisKeys &galkey,
                                     std::vector<seal::Ciphertext> &expanded_query) {
        auto N = enc_params_.poly_modulus_degree();

        auto pool = seal::MemoryManager::GetPool();

        std::cout << "Server: n_i = " << n_i << std::endl;
        std::cout << "queryExpander: expanding " << query_i.size() << " query_i ctxts" << std::endl;
        // Expand each dim according to its vector. the result should be equal `total` at most per ciphertext in that dimension. (N or n_i).
        for (uint32_t j = 0; j < query_i.size(); j++) {
            uint64_t total = N;
            if (j == query_i.size() - 1) {
                total = n_i % N; // how much more should i expand.
            }
            std::cout << "-- expanding one query_i ctxt into " << total << " ctxts " << std::endl;
            std::vector<seal::Ciphertext> expanded_query_part = __expand_query(query_i[j], total, galkey);
            expanded_query.insert(
                    expanded_query.end(),
                    std::make_move_iterator(expanded_query_part.begin()),
                    std::make_move_iterator(expanded_query_part.end()));
            expanded_query_part.clear();
        }
        std::cout << "Server: expansion done " << std::endl;
        if (expanded_query.size() != n_i) {

            std::stringstream o;
            o << " size mismatch!!! " << expanded_query.size() << ", " << n_i;
            throw std::runtime_error(o.str());
        }
    }


    inline std::vector<seal::Ciphertext>
    QueryExpander::__expand_query(const seal::Ciphertext &encrypted,
                                  uint32_t m,
                                  seal::GaloisKeys &galkey) {


        // Assume that m is a power of 2. If not, round it to the next power of 2.
        uint32_t logm = ceil(log2(m));
        seal::Plaintext two("2");

        std::vector<int> galois_elts;
        auto n = enc_params_.poly_modulus_degree();
        if (logm > ceil(log2(n))) {
            throw std::logic_error("m > n is not allowed.");
        }
        for (int i = 0; i < ceil(log2(n)); i++) {
            galois_elts.push_back((n + seal::util::exponentiate_uint(2, i)) /
                                  seal::util::exponentiate_uint(2, i));
        }

        std::vector<seal::Ciphertext> temp;
        temp.push_back(encrypted);
        seal::Ciphertext tempctxt;
        seal::Ciphertext tempctxt_rotated;
        seal::Ciphertext tempctxt_shifted;
        seal::Ciphertext tempctxt_rotatedshifted;

        for (uint32_t i = 0; i < logm - 1; i++) {
            std::vector<seal::Ciphertext> newtemp(temp.size() << 1);
            // temp[a] = (j0 = a (mod 2**i) ? ) : Enc(x^{j0 - a}) else Enc(0).  With
            // some scaling....
            int index_raw = (n << 1) - (1 << i);
            int index = (index_raw * galois_elts[i]) % (n << 1);

            for (uint32_t a = 0; a < temp.size(); a++) {

                evaluator_->apply_galois(temp[a], galois_elts[i], galkey,
                                         tempctxt_rotated);

                // cout << "rotate " <<
                // client.decryptor_->invariant_noise_budget(tempctxt_rotated) << ", ";

                evaluator_->add(temp[a], tempctxt_rotated, newtemp[a]);
                multiply_power_of_X(temp[a], tempctxt_shifted, index_raw);

                // cout << "mul by x^pow: " <<
                // client.decryptor_->invariant_noise_budget(tempctxt_shifted) << ", ";

                multiply_power_of_X(tempctxt_rotated, tempctxt_rotatedshifted, index);

                // cout << "mul by x^pow: " <<
                // client.decryptor_->invariant_noise_budget(tempctxt_rotatedshifted) <<
                // ", ";

                // Enc(2^i x^j) if j = 0 (mod 2**i).
                evaluator_->add(tempctxt_shifted, tempctxt_rotatedshifted,
                                newtemp[a + temp.size()]);
            }
            temp = newtemp;
            /*
            cout << "end: ";
            for (int h = 0; h < temp.size();h++){
                cout << client.decryptor_->invariant_noise_budget(temp[h]) << ", ";
            }
            cout << endl;
            */
        }
        // Last step of the loop
        std::vector<seal::Ciphertext> newtemp(temp.size() << 1);
        int index_raw = (n << 1) - (1 << (logm - 1));
        int index = (index_raw * galois_elts[logm - 1]) % (n << 1);
        for (uint32_t a = 0; a < temp.size(); a++) {
            if (a >= (m - (1 << (logm - 1)))) { // corner case.
                evaluator_->multiply_plain(temp[a], two,
                                           newtemp[a]); // plain multiplication by 2.
                // cout << client.decryptor_->invariant_noise_budget(newtemp[a]) << ", ";
            }
            else {
                evaluator_->apply_galois(temp[a], galois_elts[logm - 1], galkey,
                                         tempctxt_rotated);
                evaluator_->add(temp[a], tempctxt_rotated, newtemp[a]);
                multiply_power_of_X(temp[a], tempctxt_shifted, index_raw);
                multiply_power_of_X(tempctxt_rotated, tempctxt_rotatedshifted, index);
                evaluator_->add(tempctxt_shifted, tempctxt_rotatedshifted,
                                newtemp[a + temp.size()]);
            }
        }

        std::vector<seal::Ciphertext>::const_iterator first = newtemp.begin();
        std::vector<seal::Ciphertext>::const_iterator last = newtemp.begin() + m;
        std::vector<seal::Ciphertext> newVec(first, last);

        return newVec;
    }

    inline void QueryExpander::multiply_power_of_X(const seal::Ciphertext &encrypted,
                                                   seal::Ciphertext &destination,
                                                   uint32_t index) {

        auto coeff_mod_count = enc_params_.coeff_modulus().size() - 1;
        auto coeff_count = enc_params_.poly_modulus_degree();
        auto encrypted_count = encrypted.size();

        // cout << "coeff mod count for power of X = " << coeff_mod_count << endl;
        // cout << "coeff count for power of X = " << coeff_count << endl;

        // First copy over.
        destination = encrypted;

        // Prepare for destination
        // Multiply X^index for each ciphertext polynomial
        for (int i = 0; i < encrypted_count; i++) {
            for (int j = 0; j < coeff_mod_count; j++) {
                seal::util::negacyclic_shift_poly_coeffmod(encrypted.data(i) + (j * coeff_count),
                                                           coeff_count, index,
                                                           enc_params_.coeff_modulus()[j],
                                                           destination.data(i) + (j * coeff_count));
            }
        }
    }

    QueryExpander::QueryExpander(const seal::EncryptionParameters enc_params) :
            enc_params_(enc_params) {
        seal::SEALContext context(enc_params, true);

        evaluator_ = std::make_unique<seal::Evaluator>(context);
    }

    std::shared_ptr<QueryExpander>
    QueryExpander::Create(const seal::EncryptionParameters enc_params) {
        auto eval = QueryExpander(enc_params);
        return std::make_shared<QueryExpander>(std::move(eval));
    }
}