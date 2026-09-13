#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include <ma2a/contracts.hpp>
#include <ma2a/tcp.hpp>
#include <ma2a/wire.hpp>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto port = static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10));

    ma2a::JobRequest job{
        .protocol_version = "0.2",
        .message_id = "msg-tcp-1",
        .request_id = "job-tcp-1",
        .sender_node_id = "node-cpp",
        .target_node_id = "node-python",
        .organization_id = "org-1",
        .operation = "ECHO",
        .payload = "hello-over-tcp",
        .issued_at = 1700000000,
        .expires_at = 1700000060,
        .signature_algorithm = "Ed25519",
        .signature = "fixture-signature",
    };

    auto socket = ma2a::connect_ipv4("127.0.0.1", port);
    ma2a::send_framed_json(socket.get(), ma2a::job_request_envelope_json(job));
    const auto response = ma2a::recv_framed_json(socket.get());

    const std::string expected =
        "{\"payload\":{\"completed_at\":1700000001,\"message_id\":\"msg-result-1\","
        "\"payload\":\"hello-over-tcp\",\"protocol_version\":\"0.2\","
        "\"recipient_node_id\":\"node-cpp\",\"request_id\":\"job-tcp-1\","
        "\"responder_node_id\":\"node-python\",\"signature\":\"fixture-result-signature\","
        "\"signature_algorithm\":\"Ed25519\",\"status\":\"OK\"},\"type\":\"JOB_RESULT\"}";

    if (response != expected) {
        std::cerr << response << '\n';
        return 3;
    }
    return 0;
}
