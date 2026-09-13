#include <array>
#include <cassert>
#include <string>

#include <ma2a/failure_notice.hpp>

int main() {
    ma2a::FailureNotice notice{
        .protocol_version = "0.2",
        .message_id = "fail-msg-1",
        .request_id = "job-42",
        .reporting_node_id = "node-a",
        .failed_node_id = "node-b",
        .reason = "transport_timeout",
        .observed_at = 1800000100,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };

    const std::array<unsigned char, 32> seed = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    };
    const std::array<unsigned char, 32> public_key = {
        0x03,0xa1,0x07,0xbf,0xf3,0xce,0x10,0xbe,
        0x1d,0x70,0xdd,0x18,0xe7,0x4b,0xc0,0x99,
        0x67,0xe4,0xd6,0x30,0x9b,0xa5,0x0d,0x5f,
        0x1d,0xdc,0x86,0x64,0x12,0x55,0x31,0xb8,
    };

    auto priv = ma2a::ed25519_private_key_from_seed(seed);
    auto pub = ma2a::ed25519_public_key_from_raw(public_key);
    ma2a::sign_failure_notice(notice, priv.get());
    assert(ma2a::verify_failure_notice(notice, pub.get()));

    const auto envelope = ma2a::failure_notice_envelope_json(notice);
    assert(envelope.find("\"type\":\"FAILURE_NOTICE\"") != std::string::npos);
    assert(envelope.find("\"failed_node_id\":\"node-b\"") != std::string::npos);

    auto tampered = notice;
    tampered.failed_node_id = "node-c";
    assert(!ma2a::verify_failure_notice(tampered, pub.get()));
    return 0;
}
