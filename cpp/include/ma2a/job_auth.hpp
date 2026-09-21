#pragma once

#include <ma2a/canonical.hpp>
#include <ma2a/contracts.hpp>
#include <ma2a/crypto.hpp>

namespace ma2a {

inline void sign_job_request(JobRequest& request, EVP_PKEY* private_key) {
    if (!private_key) throw std::invalid_argument("private key is required");
    request.signature_algorithm = "Ed25519";
    request.signature.clear();
    request.signature = base64url_encode(
        ed25519_sign(private_key, canonical_job_request_json(request))
    );
}

inline void sign_job_result(JobResult& result, EVP_PKEY* private_key) {
    if (!private_key) throw std::invalid_argument("private key is required");
    result.signature_algorithm = "Ed25519";
    result.signature.clear();
    result.signature = base64url_encode(
        ed25519_sign(private_key, canonical_job_result_json(result))
    );
}

inline bool verify_job_result(const JobResult& result, EVP_PKEY* public_key) {
    if (!public_key || result.signature_algorithm != "Ed25519" || result.signature.empty())
        return false;
    try {
        return ed25519_verify(
            public_key,
            canonical_job_result_json(result),
            base64url_decode(result.signature)
        );
    } catch (...) {
        return false;
    }
}

} // namespace ma2a
