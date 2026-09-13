#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/evp.h>

namespace ma2a {

using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using EvpMdCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

inline std::string base64url_encode(const std::vector<unsigned char>& input) {
    if (input.empty()) return {};
    std::string output(4 * ((input.size() + 2) / 3), '\0');
    const int len = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(output.data()),
        input.data(),
        static_cast<int>(input.size())
    );
    if (len < 0) throw std::runtime_error("base64 encode failed");
    output.resize(static_cast<std::size_t>(len));
    std::replace(output.begin(), output.end(), '+', '-');
    std::replace(output.begin(), output.end(), '/', '_');
    return output;
}

inline std::vector<unsigned char> base64url_decode(std::string input) {
    std::replace(input.begin(), input.end(), '-', '+');
    std::replace(input.begin(), input.end(), '_', '/');
    if (input.size() % 4 != 0) throw std::invalid_argument("invalid base64 length");
    std::vector<unsigned char> output(3 * (input.size() / 4));
    const int len = EVP_DecodeBlock(
        output.data(),
        reinterpret_cast<const unsigned char*>(input.data()),
        static_cast<int>(input.size())
    );
    if (len < 0) throw std::invalid_argument("invalid base64");
    std::size_t actual = static_cast<std::size_t>(len);
    if (!input.empty() && input.back() == '=') --actual;
    if (input.size() >= 2 && input[input.size() - 2] == '=') --actual;
    output.resize(actual);
    return output;
}

inline EvpPkeyPtr ed25519_private_key_from_seed(const std::array<unsigned char, 32>& seed) {
    EVP_PKEY* raw = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    if (!raw) throw std::runtime_error("failed to create Ed25519 private key");
    return EvpPkeyPtr(raw, EVP_PKEY_free);
}

inline EvpPkeyPtr ed25519_public_key_from_raw(const std::array<unsigned char, 32>& key) {
    EVP_PKEY* raw = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, key.data(), key.size());
    if (!raw) throw std::runtime_error("failed to create Ed25519 public key");
    return EvpPkeyPtr(raw, EVP_PKEY_free);
}

inline std::vector<unsigned char> ed25519_sign(EVP_PKEY* private_key, const std::string& message) {
    EvpMdCtxPtr ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) throw std::runtime_error("failed to allocate digest context");
    if (EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, private_key) != 1)
        throw std::runtime_error("Ed25519 sign init failed");
    std::size_t signature_len = 0;
    if (EVP_DigestSign(ctx.get(), nullptr, &signature_len,
                       reinterpret_cast<const unsigned char*>(message.data()), message.size()) != 1)
        throw std::runtime_error("Ed25519 signature sizing failed");
    std::vector<unsigned char> signature(signature_len);
    if (EVP_DigestSign(ctx.get(), signature.data(), &signature_len,
                       reinterpret_cast<const unsigned char*>(message.data()), message.size()) != 1)
        throw std::runtime_error("Ed25519 signing failed");
    signature.resize(signature_len);
    return signature;
}

inline bool ed25519_verify(EVP_PKEY* public_key, const std::string& message,
                           const std::vector<unsigned char>& signature) {
    EvpMdCtxPtr ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) throw std::runtime_error("failed to allocate digest context");
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, public_key) != 1)
        throw std::runtime_error("Ed25519 verify init failed");
    return EVP_DigestVerify(
        ctx.get(), signature.data(), signature.size(),
        reinterpret_cast<const unsigned char*>(message.data()), message.size()
    ) == 1;
}

} // namespace ma2a
