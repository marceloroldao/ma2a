#pragma once

#include <stdexcept>
#include <string>

#include <ma2a/canonical.hpp>
#include <ma2a/contracts.hpp>

namespace ma2a {

inline std::string signed_job_request_json(const JobRequest& job) {
    const auto unsigned_json = canonical_job_request_json(job);
    return unsigned_json.substr(0, unsigned_json.size() - 1)
        + ",\"signature\":" + json_string(job.signature) + "}";
}

inline std::string signed_job_result_json(const JobResult& result) {
    // `signature` sorts between `signature_algorithm` and `status` in Python.
    const auto unsigned_json = canonical_job_result_json(result);
    const auto status_marker = std::string{",\"status\":"};
    const auto status_pos = unsigned_json.find(status_marker);
    if (status_pos == std::string::npos) {
        throw std::logic_error("canonical JobResult shape changed");
    }
    return unsigned_json.substr(0, status_pos)
        + ",\"signature\":" + json_string(result.signature)
        + unsigned_json.substr(status_pos);
}

inline std::string job_request_envelope_json(const JobRequest& job) {
    // Python transport uses sort_keys=True, so envelope order is payload, type.
    return "{\"payload\":" + signed_job_request_json(job) + ",\"type\":\"JOB_REQUEST\"}";
}

inline std::string job_result_envelope_json(const JobResult& result) {
    return "{\"payload\":" + signed_job_result_json(result) + ",\"type\":\"JOB_RESULT\"}";
}

} // namespace ma2a
