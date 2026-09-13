#pragma once

#include <sstream>
#include <string>

#include <ma2a/canonical.hpp>
#include <ma2a/contracts.hpp>
#include <ma2a/crypto.hpp>

namespace ma2a {

inline std::string canonical_failure_notice_json(const FailureNotice& notice) {
    std::ostringstream out;
    out << '{'
        << "\"failed_node_id\":" << json_string(notice.failed_node_id) << ','
        << "\"message_id\":" << json_string(notice.message_id) << ','
        << "\"observed_at\":" << notice.observed_at << ','
        << "\"protocol_version\":" << json_string(notice.protocol_version) << ','
        << "\"reason\":" << json_string(notice.reason) << ','
        << "\"reporting_node_id\":" << json_string(notice.reporting_node_id) << ','
        << "\"request_id\":" << json_string(notice.request_id) << ','
        << "\"signature_algorithm\":" << json_string(notice.signature_algorithm)
        << '}';
    return out.str();
}

inline std::string signed_failure_notice_json(const FailureNotice& notice) {
    std::ostringstream out;
    out << '{'
        << "\"failed_node_id\":" << json_string(notice.failed_node_id) << ','
        << "\"message_id\":" << json_string(notice.message_id) << ','
        << "\"observed_at\":" << notice.observed_at << ','
        << "\"protocol_version\":" << json_string(notice.protocol_version) << ','
        << "\"reason\":" << json_string(notice.reason) << ','
        << "\"reporting_node_id\":" << json_string(notice.reporting_node_id) << ','
        << "\"request_id\":" << json_string(notice.request_id) << ','
        << "\"signature\":" << json_string(notice.signature) << ','
        << "\"signature_algorithm\":" << json_string(notice.signature_algorithm)
        << '}';
    return out.str();
}

inline std::string failure_notice_envelope_json(const FailureNotice& notice) {
    return "{\"payload\":" + signed_failure_notice_json(notice) + ",\"type\":\"FAILURE_NOTICE\"}";
}

inline void sign_failure_notice(FailureNotice& notice, EVP_PKEY* private_key) {
    notice.signature_algorithm = "Ed25519";
    notice.signature = base64url_encode(ed25519_sign(private_key, canonical_failure_notice_json(notice)));
}

inline bool verify_failure_notice(const FailureNotice& notice, EVP_PKEY* public_key) {
    if (notice.signature_algorithm != "Ed25519" || notice.signature.empty()) return false;
    try {
        return ed25519_verify(
            public_key,
            canonical_failure_notice_json(notice),
            base64url_decode(notice.signature)
        );
    } catch (...) {
        return false;
    }
}

} // namespace ma2a
