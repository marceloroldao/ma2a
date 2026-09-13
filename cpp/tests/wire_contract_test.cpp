#include <cassert>
#include <stdexcept>
#include <string>

#include <ma2a/contracts.hpp>
#include <ma2a/framing.hpp>
#include <ma2a/wire.hpp>

int main() {
    ma2a::JobRequest job{
        .protocol_version = "0.2",
        .message_id = "msg-interop-1",
        .request_id = "job-interop-1",
        .sender_node_id = "node-python",
        .target_node_id = "node-cpp",
        .organization_id = "org-1",
        .operation = "ECHO",
        .payload = "hello",
        .issued_at = 1700000000,
        .expires_at = 1700000060,
        .signature_algorithm = "Ed25519",
        .signature = "abc123==",
    };

    const std::string expected =
        "{\"payload\":{\"expires_at\":1700000060,\"issued_at\":1700000000,"
        "\"message_id\":\"msg-interop-1\",\"operation\":\"ECHO\","
        "\"organization_id\":\"org-1\",\"payload\":\"hello\","
        "\"protocol_version\":\"0.2\",\"request_id\":\"job-interop-1\","
        "\"sender_node_id\":\"node-python\",\"signature\":\"abc123==\","
        "\"signature_algorithm\":\"Ed25519\",\"target_node_id\":\"node-cpp\"},"
        "\"type\":\"JOB_REQUEST\"}";

    const auto wire = ma2a::job_request_envelope_json(job);
    assert(wire == expected);
    const auto frame = ma2a::encode_frame(wire);
    assert(ma2a::decode_complete_frame(frame) == expected);

    bool empty_rejected = false;
    try { (void)ma2a::encode_frame(""); }
    catch (const std::invalid_argument&) { empty_rejected = true; }
    assert(empty_rejected);

    bool oversize_rejected = false;
    try { (void)ma2a::encode_frame(std::string(ma2a::kMaxFrameBytes + 1, 'x')); }
    catch (const std::length_error&) { oversize_rejected = true; }
    assert(oversize_rejected);
    return 0;
}
