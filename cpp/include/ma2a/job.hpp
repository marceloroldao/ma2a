#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include <ma2a/contracts.hpp>

namespace ma2a {

inline void validate_job_request(const JobRequest& request, std::int64_t now = -1) {
    if (request.protocol_version.empty() || request.message_id.empty() || request.request_id.empty() ||
        request.sender_node_id.empty() || request.target_node_id.empty() || request.operation.empty()) {
        throw std::invalid_argument("missing job request field");
    }
    if (request.operation != "PING" && request.operation != "ECHO") {
        throw std::invalid_argument("unsupported job operation");
    }
    if (request.issued_at < 0 || request.expires_at <= request.issued_at) {
        throw std::invalid_argument("invalid job validity window");
    }
    if (now >= 0 && !(request.issued_at <= now && now <= request.expires_at)) {
        throw std::invalid_argument("job request expired or not yet valid");
    }
    if (request.signature_algorithm != "Ed25519") {
        throw std::invalid_argument("unsupported signature algorithm");
    }
}

inline void validate_job_result(const JobResult& result) {
    if (result.protocol_version.empty() || result.message_id.empty() || result.request_id.empty() ||
        result.responder_node_id.empty() || result.recipient_node_id.empty() || result.status.empty()) {
        throw std::invalid_argument("missing job result field");
    }
    if (result.status != "OK" && result.status != "ERROR") {
        throw std::invalid_argument("invalid job result status");
    }
    if (result.completed_at < 0) {
        throw std::invalid_argument("negative completion time");
    }
    if (result.signature_algorithm != "Ed25519") {
        throw std::invalid_argument("unsupported signature algorithm");
    }
}

inline std::pair<std::string, std::string> execute_reference_job(const JobRequest& request) {
    if (request.operation == "PING") return {"OK", "PONG"};
    if (request.operation == "ECHO") return {"OK", request.payload};
    return {"ERROR", "unsupported_operation"};
}

} // namespace ma2a
