#include <cassert>
#include <stdexcept>
#include <string>
#include <ma2a/framing.hpp>
#include <ma2a/job.hpp>

int main() {
    ma2a::JobRequest ping{
        .protocol_version = "0.2",
        .message_id = "msg-1",
        .request_id = "job-1",
        .sender_node_id = "node-a",
        .target_node_id = "node-b",
        .organization_id = "org-1",
        .operation = "PING",
        .payload = "",
        .issued_at = 100,
        .expires_at = 200,
    };
    ma2a::validate_job_request(ping, 150);
    auto [status, payload] = ma2a::execute_reference_job(ping);
    assert(status == "OK");
    assert(payload == "PONG");

    ping.operation = "ECHO";
    ping.payload = "hello";
    ma2a::validate_job_request(ping, 150);
    auto echoed = ma2a::execute_reference_job(ping);
    assert(echoed.first == "OK");
    assert(echoed.second == "hello");

    auto frame = ma2a::encode_frame("{\"type\":\"job\"}");
    assert(ma2a::decode_complete_frame(frame) == "{\"type\":\"job\"}");

    bool rejected = false;
    try {
        ping.operation = "SHELL";
        ma2a::validate_job_request(ping, 150);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
    return 0;
}
