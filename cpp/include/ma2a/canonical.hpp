#pragma once

#include <iomanip>
#include <sstream>
#include <string>

#include <ma2a/contracts.hpp>

namespace ma2a {

inline std::string json_string(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(ch) << std::dec;
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    out << '"';
    return out.str();
}

inline std::string canonical_job_request_json(const JobRequest& job) {
    // Key order exactly matches Python json.dumps(..., sort_keys=True, separators=(",", ":")).
    std::ostringstream out;
    out << '{'
        << "\"expires_at\":" << job.expires_at << ','
        << "\"issued_at\":" << job.issued_at << ','
        << "\"message_id\":" << json_string(job.message_id) << ','
        << "\"operation\":" << json_string(job.operation) << ','
        << "\"organization_id\":";
    if (job.organization_id.empty()) out << "null";
    else out << json_string(job.organization_id);
    out << ','
        << "\"payload\":" << json_string(job.payload) << ','
        << "\"protocol_version\":" << json_string(job.protocol_version) << ','
        << "\"request_id\":" << json_string(job.request_id) << ','
        << "\"sender_node_id\":" << json_string(job.sender_node_id) << ','
        << "\"signature_algorithm\":" << json_string(job.signature_algorithm) << ','
        << "\"target_node_id\":" << json_string(job.target_node_id)
        << '}';
    return out.str();
}

inline std::string canonical_job_result_json(const JobResult& result) {
    std::ostringstream out;
    out << '{'
        << "\"completed_at\":" << result.completed_at << ','
        << "\"message_id\":" << json_string(result.message_id) << ','
        << "\"payload\":" << json_string(result.payload) << ','
        << "\"protocol_version\":" << json_string(result.protocol_version) << ','
        << "\"recipient_node_id\":" << json_string(result.recipient_node_id) << ','
        << "\"request_id\":" << json_string(result.request_id) << ','
        << "\"responder_node_id\":" << json_string(result.responder_node_id) << ','
        << "\"signature_algorithm\":" << json_string(result.signature_algorithm) << ','
        << "\"status\":" << json_string(result.status)
        << '}';
    return out.str();
}

} // namespace ma2a
