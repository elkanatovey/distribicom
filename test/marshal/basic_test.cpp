#include "../test_utils.hpp"
#include "marshal.hpp"

int basic_test(int, char *[]) {
    auto all = TestUtils::setup(TestUtils::DEFAULT_SETUP_CONFIGS);

    Marshaller m(all->encryption_params);


    auto ptx = all->random_plaintext();
    vector<seal::seal_byte> tmp(ptx.save_size(), static_cast<const byte>('a'));
    ptx.save((seal::seal_byte *) &tmp[0], tmp.size());

    seal::Plaintext newptx;
    m.unmarshal_seal_object<seal::Plaintext>(tmp, newptx);
    assert(newptx.to_string() == ptx.to_string());

    // checking ctx marshalling:
    auto ctx = all->random_ciphertext();
    vector<seal::seal_byte> tmpctx(ctx.save_size(), static_cast<const byte>('a'));
    ctx.save((seal::seal_byte *) &tmpctx[0], tmpctx.size());

    seal::Ciphertext newctx;
    m.unmarshal_seal_object<seal::Ciphertext>(tmpctx, newctx);
    all->w_evaluator->evaluator->sub_inplace(newctx, ctx);
    assert(newctx.is_transparent());

    return 0;
}