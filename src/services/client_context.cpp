#include "client_context.hpp"

void
services::set_client(std::uint32_t expansion_ratio, std::uint32_t db_rows, int client_id, const seal::GaloisKeys &gkey,
                     std::string &gkey_serialised, std::vector<std::vector<seal::Ciphertext>> &query,
                     const distribicom::ClientQueryRequest &query_marshaled,
                     std::unique_ptr<services::ClientInfo> &client_info) {
    client_info->galois_keys = gkey;
    client_info->query = std::move(query);

    client_info->galois_keys_marshaled.set_keys(gkey_serialised);
    client_info->galois_keys_marshaled.set_key_pos(client_id);
    client_info->query_info_marshaled.CopyFrom(query_marshaled);
    client_info->query_info_marshaled.set_mailbox_id(client_id);
    client_info->answer_count=0;
    client_info->partial_answer =
            std::make_unique<math_utils::matrix<seal::Plaintext>>(math_utils::matrix<seal::Plaintext>
                                                                     (db_rows, expansion_ratio));
    client_info->final_answer =
            std::make_unique<math_utils::matrix<seal::Ciphertext>>(math_utils::matrix<seal::Ciphertext>
                                                                      (1, expansion_ratio));
}
