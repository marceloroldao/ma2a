#include <cassert>
#include <string>

#include <ma2a/contracts.hpp>
#include <ma2a/wire.hpp>

int main() {
    ma2a::JobResult result{
        .protocol_version = "0.2",
        .message_id = "result-1",
        .request_id = "job-interop-1",
        .responder_node_id = "node-cpp",
        .recipient_node_id = "node-python",
        .status = "OK",
        .payload = "hello",
        .completed_at = 1700000001,
        .signature_algorithm = "Ed25519",
        .signature = "xyz789==",
    };

    const std::string expected =
        "{\"payload\":{\"completed_at\":1700000001,\"message_id\":\"result-1\","
        "\"payload\":\"hello\",\"protocol_version\":\"0.2\","
        "\"recipient_node_id\":\"node-python\",\"request_id\":\"job-interop-1\","
        "\"responder_node_id\":\"node-cpp\",\"signature\":\"xyz789==\","
        "\"signature_algorithm\":\"Ed25519\",\"status\":\"OK\"},"
        "\"type\":\"JOB_RESULT\"}";

    assert(ma2a::job_result_envelope_json(result) == expected);
    return 0;
}
