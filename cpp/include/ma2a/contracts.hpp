#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ma2a {

enum class Scope {
    LocalOnly,
    Private,
    Organization,
    Public,
};

struct CapabilityAdvertisement {
    std::string protocol_version;
    std::string message_id;
    std::string node_id;
    std::string organization_id;
    std::uint64_t sequence{};
    std::int64_t issued_at{};
    std::int64_t expires_at{};
    bool available{};
    double compute_capacity{};
    double current_load{};
    std::vector<std::string> models;
    std::vector<std::string> memory_domains;
    std::vector<Scope> supported_scopes;
    std::string signature_algorithm{"Ed25519"};
    std::string signature;
};

struct JobRequest {
    std::string protocol_version;
    std::string message_id;
    std::string request_id;
    std::string sender_node_id;
    std::string target_node_id;
    std::string organization_id;
    std::string operation;
    std::string payload;
    std::int64_t issued_at{};
    std::int64_t expires_at{};
    std::string signature_algorithm{"Ed25519"};
    std::string signature;
};

struct JobResult {
    std::string protocol_version;
    std::string message_id;
    std::string request_id;
    std::string responder_node_id;
    std::string recipient_node_id;
    std::string status;
    std::string payload;
    std::int64_t completed_at{};
    std::string signature_algorithm{"Ed25519"};
    std::string signature;
};

struct FailureNotice {
    std::string protocol_version;
    std::string message_id;
    std::string request_id;
    std::string reporting_node_id;
    std::string failed_node_id;
    std::string reason;
    std::int64_t observed_at{};
    std::string signature_algorithm{"Ed25519"};
    std::string signature;
};

} // namespace ma2a
