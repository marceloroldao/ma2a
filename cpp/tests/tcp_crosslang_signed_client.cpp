#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include <ma2a/canonical.hpp>
#include <ma2a/contracts.hpp>
#include <ma2a/crypto.hpp>
#include <ma2a/tcp.hpp>
#include <ma2a/wire.hpp>

namespace {
std::string extract_string_field(const std::string& json, const std::string& key) {
    const auto marker = std::string{"\""} + key + "\":\"";
    const auto start = json.find(marker);
    if (start == std::string::npos) return {};
    const auto value_start = start + marker.size();
    const auto end = json.find('"', value_start);
    if (end == std::string::npos) return {};
    return json.substr(value_start, end - value_start);
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto port = static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10));

    ma2a::JobRequest job{
        .protocol_version = "0.2",
        .message_id = "msg-signed-tcp-1",
        .request_id = "job-signed-tcp-1",
        .sender_node_id = "node-cpp",
        .target_node_id = "node-python",
        .organization_id = "org-1",
        .operation = "ECHO",
        .payload = "signed-hello",
        .issued_at = 1800000000,
        .expires_at = 1800000060,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };

    const std::array<unsigned char, 32> cpp_seed = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    };
    auto cpp_private = ma2a::ed25519_private_key_from_seed(cpp_seed);
    job.signature = ma2a::base64url_encode(
        ma2a::ed25519_sign(cpp_private.get(), ma2a::canonical_job_request_json(job))
    );

    auto socket = ma2a::connect_ipv4("127.0.0.1", port);
    ma2a::send_framed_json(socket.get(), ma2a::job_request_envelope_json(job));
    const auto response = ma2a::recv_framed_json(socket.get());

    ma2a::JobResult result{
        .protocol_version = extract_string_field(response, "protocol_version"),
        .message_id = extract_string_field(response, "message_id"),
        .request_id = extract_string_field(response, "request_id"),
        .responder_node_id = extract_string_field(response, "responder_node_id"),
        .recipient_node_id = extract_string_field(response, "recipient_node_id"),
        .status = extract_string_field(response, "status"),
        .payload = extract_string_field(response, "payload"),
        .completed_at = 1800000001,
        .signature_algorithm = extract_string_field(response, "signature_algorithm"),
        .signature = extract_string_field(response, "signature"),
    };

    if (result.request_id != job.request_id || result.payload != job.payload || result.status != "OK") return 3;

    const std::array<unsigned char, 32> python_public = {
        0x29,0xac,0xba,0xe1,0x41,0xbc,0xca,0xf0,
        0xb2,0x2e,0x1a,0x94,0xd3,0x4d,0x0b,0xc7,
        0x36,0x1e,0x52,0x6d,0x0b,0xfe,0x12,0xc8,
        0x97,0x94,0xbc,0x93,0x22,0x96,0x6d,0xd7,
    };
    auto python_pub = ma2a::ed25519_public_key_from_raw(python_public);
    const auto signature = ma2a::base64url_decode(result.signature);
    if (!ma2a::ed25519_verify(
            python_pub.get(),
            ma2a::canonical_job_result_json(result),
            signature)) {
        std::cerr << "python result signature verification failed\n";
        return 4;
    }
    return 0;
}
