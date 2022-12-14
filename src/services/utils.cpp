#include "utils.hpp"
#include <charconv>
#include <string>

namespace services::utils {
    int extract_size_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                   const services::constants::metadata &md) {
        auto ntmp = mp.find(std::string(md));
        return std::stoi(std::string(ntmp->second.data(), ntmp->second.size()));
    }

    void add_metadata_size(grpc::ClientContext &context, const services::constants::metadata &md, int size) {
        context.AddMetadata(std::string(md), std::to_string(size));
    }

    const std::string ipv4("ipv4:");
    const std::string ipv6("ipv6:");

    char from_hex(char ch) {
        return isdigit(ch) ? char(ch - '0') : char(tolower(ch) - 'a' + 10);
    }

    // help understanding in: https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
    std::string url_decode(std::string text) {

        std::ostringstream decoded;
        for (auto i = text.begin(), end = text.end(); i != end; ++i) {
            std::string::value_type c = (*i);

            switch (c) {
                case '%':
                    if (i + 1 == end || i + 2 == end) {
                        return decoded.str();
                    }

                    // decoding from 2 hex values into a single char:
                    decoded << char(from_hex(i[1]) << 4 | from_hex(i[2]));
                    i += 2;

                    break;
                case '+':
                    decoded << ' ';
                    break;
                default:
                    decoded << c;
            }

        }

        return decoded.str();
    }


    std::string extract_ip(grpc::ServerContext *pContext) {
        return extract_ip(std::string(pContext->peer()));
    }

    std::string extract_ip(std::string fullip) {

        if (fullip.find(ipv4) != std::string::npos) {
            fullip.erase(0, fullip.find(ipv4) + ipv4.length());
            return fullip.substr(0, fullip.find(':'));
        }

        // assuming ipv6:
        fullip = url_decode(fullip);

        fullip.erase(0, fullip.find(ipv6) + ipv6.length());
        fullip.substr(0, fullip.find(':'));

        if (fullip.find(']') != std::string::npos) {
            fullip.erase(fullip.find(']'), fullip.length());
            fullip.erase(0, fullip.find('[') + 1);
        }

        return fullip;
    }

    seal::EncryptionParameters setup_enc_params(const distribicom::AppConfigs &cnfgs) {
        seal::EncryptionParameters enc_params(seal::scheme_type::bgv);
        auto N = cnfgs.configs().polynomial_degree();
        auto logt = cnfgs.configs().logarithm_plaintext_coefficient();

        enc_params.set_poly_modulus_degree(N);
        // TODO: Use correct BGV parameters.
        enc_params.set_coeff_modulus(seal::CoeffModulus::BFVDefault(N));
        enc_params.set_plain_modulus(seal::PlainModulus::Batching(N, logt + 1));
        return enc_params;
    }

    void add_metadata_string(grpc::ClientContext &context, const constants::metadata &md, const std::string &str) {
        context.AddMetadata(std::string(md), str);
    }

    std::string extract_string_from_metadata(const std::multimap<grpc::string_ref, grpc::string_ref> &mp,
                                             const constants::metadata &md) {
        auto ntmp = mp.find(std::string(md));
        return {ntmp->second.data(), ntmp->second.size()};
    }


    static const unsigned char base64_table[65] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /**
    * base64_encode - Base64 encode
    * @src: Data to be encoded
    * @len: Length of the data to be encoded
    * @out_len: Pointer to output length variable, or %NULL if not used
    * Returns: Allocated buffer of out_len bytes of encoded data,
    * or empty string on failure
    */
    std::string base64_encode(const unsigned char *src, size_t len) {
        unsigned char *out, *pos;
        const unsigned char *end, *in;

        size_t olen;

        olen = 4 * ((len + 2) / 3); /* 3-byte blocks to 4-byte */

        if (olen < len)
            return std::string(); /* integer overflow */

        std::string outStr;
        outStr.resize(olen);
        out = (unsigned char *) &outStr[0];

        end = src + len;
        in = src;
        pos = out;
        while (end - in >= 3) {
            *pos++ = base64_table[in[0] >> 2];
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
            *pos++ = base64_table[in[2] & 0x3f];
            in += 3;
        }

        if (end - in) {
            *pos++ = base64_table[in[0] >> 2];
            if (end - in == 1) {
                *pos++ = base64_table[(in[0] & 0x03) << 4];
                *pos++ = '=';
            } else {
                *pos++ = base64_table[((in[0] & 0x03) << 4) |
                                      (in[1] >> 4)];
                *pos++ = base64_table[(in[1] & 0x0f) << 2];
            }
            *pos++ = '=';
        }

        return outStr;
    }

    std::string byte_vec_to_64base_string(const std::vector<std::byte> &bytes) {
        return base64_encode((unsigned char *) (&bytes[0]), bytes.size());
    }
}

