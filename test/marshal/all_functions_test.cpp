#include "../test_utils.hpp"
#include "marshal.hpp"

void plaintext_check(std::shared_ptr<TestUtils::CryptoObjects> &all, std::shared_ptr<marshal::Marshaller> &m) {
    auto ptx = all->random_plaintext();
    std::vector<seal::seal_byte> tmp(ptx.save_size());
    ptx.save((seal::seal_byte *) &tmp[0], tmp.size());

    // test plaintext unmarshalling
    seal::Plaintext newptx;
    m->unmarshal_seal_object<seal::Plaintext>(tmp, newptx);
    assert(newptx.to_string() == ptx.to_string());


    //test plaintext marshaling
    seal::Plaintext unmarshaled_m_ptx;
    std::vector<seal::seal_byte> m_ptx(ptx.save_size());
    m->marshal_seal_object(ptx, m_ptx);

    m->unmarshal_seal_object<seal::Plaintext>(m_ptx, unmarshaled_m_ptx);
    assert(unmarshaled_m_ptx.to_string() == ptx.to_string());
}

void ciphertext_check(std::shared_ptr<TestUtils::CryptoObjects> &all,
                      const std::shared_ptr<marshal::Marshaller> &m) {// checking ctx unmarshalling:
    auto ctx = all->random_ciphertext();
    std::vector<seal::seal_byte> tmpctx(ctx.save_size());
    ctx.save((seal::seal_byte *) &tmpctx[0], tmpctx.size());

    seal::Ciphertext newctx;
    m->unmarshal_seal_object<seal::Ciphertext>(tmpctx, newctx);
    all->w_evaluator->evaluator->sub_inplace(newctx, ctx);
    assert(newctx.is_transparent());

    // check ctext marshal
    seal::Ciphertext unmarshaled_m_ctx;
    std::vector<seal::seal_byte> m_ctx(ctx.save_size());
    m->marshal_seal_object(ctx, m_ctx);

    m->unmarshal_seal_object<seal::Ciphertext>(m_ctx, unmarshaled_m_ctx);
    all->w_evaluator->evaluator->sub_inplace(unmarshaled_m_ctx, ctx);
    assert(unmarshaled_m_ctx.is_transparent());
}

void assert_galois_keys_equals(const std::shared_ptr<TestUtils::CryptoObjects> &all, const seal::GaloisKeys &g_key,
                               const seal::GaloisKeys &newgkey) {
    assert(false); // unable to compare.
}

void galois_keys_check(std::shared_ptr<TestUtils::CryptoObjects> &all, const std::shared_ptr<marshal::Marshaller> &m) {
    auto g_key = all->generate_galois_keys();

    //check gkey unmarshal
    std::vector<seal::seal_byte> tmp_gkey(g_key.save_size());
    g_key.save((seal::seal_byte *) &tmp_gkey[0], tmp_gkey.size());

    seal::GaloisKeys newgkey;
    m->unmarshal_seal_object<seal::GaloisKeys>(tmp_gkey, newgkey);
    assert(seal::is_valid_for(newgkey, all->seal_context));

    //check gkey marshal
    seal::GaloisKeys unmarshaled_m_gkey;
    std::vector<seal::seal_byte> m_gkey(g_key.save_size());
    m->marshal_seal_object(g_key, m_gkey);

    m->unmarshal_seal_object<seal::GaloisKeys>(m_gkey, unmarshaled_m_gkey);
    assert(seal::is_valid_for(unmarshaled_m_gkey, all->seal_context));

    assert_galois_keys_equals(all, g_key, newgkey);
}

void plaintext_vector_check(std::shared_ptr<TestUtils::CryptoObjects> &all,
                            std::shared_ptr<marshal::Marshaller> &m) {
    // creating random plaintexts
    std::vector<seal::Plaintext> ptxs(100);
    for (auto &ptx: ptxs) {
        ptx = all->random_plaintext();
    }
    std::vector<marshal::marshaled_seal_object> marshaled(ptxs.size());
    m->marshal_seal_vector(ptxs, marshaled);

    std::vector<seal::Plaintext> unmarshaled_ptxs(marshaled.size());
    m->unmarshal_seal_vector(marshaled, unmarshaled_ptxs);

    for (std::uint64_t i = 0; i < unmarshaled_ptxs.size(); ++i) {
        assert(unmarshaled_ptxs[i].to_string() == ptxs[i].to_string());
    }
}

void ciphertext_vector_check(std::shared_ptr<TestUtils::CryptoObjects> &all,
                             std::shared_ptr<marshal::Marshaller> &m) {
    // creating random ciphertexts
    std::vector<seal::Ciphertext> ctxs(100);
    for (auto &ctx: ctxs) {
        ctx = all->random_ciphertext();
    }
    std::vector<marshal::marshaled_seal_object> marshaled(ctxs.size());
    m->marshal_seal_vector(ctxs, marshaled);

    std::vector<seal::Ciphertext> unmarshaled_ctxs(marshaled.size());
    m->unmarshal_seal_vector(marshaled, unmarshaled_ctxs);

    for (std::uint64_t i = 0; i < unmarshaled_ctxs.size(); ++i) {
        all->w_evaluator->evaluator->sub_inplace(unmarshaled_ctxs[i], ctxs[i]);
        assert(unmarshaled_ctxs[i].is_transparent());
    }
}

int all_functions_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    auto m = marshal::Marshaller::Create(all->encryption_params);

    plaintext_check(all, m);
    ciphertext_check(all, m);
//    galois_keys_check(all, m);

    plaintext_vector_check(all, m);
    ciphertext_vector_check(all, m);
    return 0;
}