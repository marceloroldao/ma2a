#include <array>
#include <cassert>
#include <string>

#include <ma2a/canonical.hpp>
#include <ma2a/contracts.hpp>
#include <ma2a/crypto.hpp>

int main() {
    ma2a::JobRequest job{
        .protocol_version = "0.2",
        .message_id = "msg-interop-1",
        .request_id = "job-interop-1",
        .sender_node_id = "node-python",
        .target_node_id = "node-cpp",
        .organization_id = "org-1",
        .operation = "PING",
        .payload = "",
        .issued_at = 1800000000,
        .expires_at = 1800000030,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };

    const std::string expected_json =
        "{\"expires_at\":1800000030,\"issued_at\":1800000000,\"message_id\":\"msg-interop-1\","
        "\"operation\":\"PING\",\"organization_id\":\"org-1\",\"payload\":\"\","
        "\"protocol_version\":\"0.2\",\"request_id\":\"job-interop-1\","
        "\"sender_node_id\":\"node-python\",\"signature_algorithm\":\"Ed25519\","
        "\"target_node_id\":\"node-cpp\"}";
    const auto canonical = ma2a::canonical_job_request_json(job);
    assert(canonical == expected_json);

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
    const std::string python_signature =
        "tzGr6lzGR4RM9s0yLKRXXm4X5jvNl6lmid-Y5QXKGezmfdQkIFTVCwJzJ6fGhh9LJjja7mZCOAEKyGA4CRFMBQ==";

    auto private_key = ma2a::ed25519_private_key_from_seed(seed);
    auto cpp_signature = ma2a::ed25519_sign(private_key.get(), canonical);
    assert(ma2a::base64url_encode(cpp_signature) == python_signature);

    auto pub = ma2a::ed25519_public_key_from_raw(public_key);
    const auto decoded_python_signature = ma2a::base64url_decode(python_signature);
    assert(ma2a::ed25519_verify(pub.get(), canonical, decoded_python_signature));

    std::string tampered = canonical;
    tampered.back() = ' ';
    assert(!ma2a::ed25519_verify(pub.get(), tampered, decoded_python_signature));
    return 0;
}
