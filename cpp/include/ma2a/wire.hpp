#pragma once

#include <sstream>
#include <string>

#include <ma2a/canonical.hpp>
#include <ma2a/contracts.hpp>

namespace ma2a {

inline std::string signed_job_request_json(const JobRequest& job) {
    // Exact Python json.dumps(asdict(job), sort_keys=True, separators=(",", ":")) order.
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
        << "\"signature\":" << json_string(job.signature) << ','
        << "\"signature_algorithm\":" << json_string(job.signature_algorithm) << ','
        << "\"target_node_id\":" << json_string(job.target_node_id)
        << '}';
    return out.str();
}

inline std::string signed_job_result_json(const JobResult& result) {
    std::ostringstream out;
    out << '{'
        << "\"completed_at\":" << result.completed_at << ','
        << "\"message_id\":" << json_string(result.message_id) << ','
        << "\"payload\":" << json_string(result.payload) << ','
        << "\"protocol_version\":" << json_string(result.protocol_version) << ','
        << "\"recipient_node_id\":" << json_string(result.recipient_node_id) << ','
        << "\"request_id\":" << json_string(result.request_id) << ','
        << "\"responder_node_id\":" << json_string(result.responder_node_id) << ','
        << "\"signature\":" << json_string(result.signature) << ','
        << "\"signature_algorithm\":" << json_string(result.signature_algorithm) << ','
        << "\"status\":" << json_string(result.status)
        << '}';
    return out.str();
}

inline std::string job_request_envelope_json(const JobRequest& job) {
    return "{\"payload\":" + signed_job_request_json(job) + ",\"type\":\"JOB_REQUEST\"}";
}

inline std::string job_result_envelope_json(const JobResult& result) {
    return "{\"payload\":" + signed_job_result_json(result) + ",\"type\":\"JOB_RESULT\"}";
}

} // namespace ma2a
