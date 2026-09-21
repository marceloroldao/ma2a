#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include <ma2a/failure_notice.hpp>
#include <ma2a/job_auth.hpp>
#include <ma2a/resilient_execution.hpp>
#include <ma2a/tcp.hpp>
#include <ma2a/wire.hpp>
#include <ma2a/wire_parse.hpp>

namespace ma2a {

struct TcpEndpoint {
    std::string host;
    std::uint16_t port{};
};

using EndpointResolver = std::function<std::optional<TcpEndpoint>(const std::string&)>;
using PublicKeyResolver = std::function<EVP_PKEY*(const std::string&)>;

class AuthenticatedTcpAttemptExecutor {
public:
    AuthenticatedTcpAttemptExecutor(
        EVP_PKEY* local_private_key,
        EndpointResolver endpoint_resolver,
        PublicKeyResolver public_key_resolver)
        : local_private_key_(local_private_key),
          endpoint_resolver_(std::move(endpoint_resolver)),
          public_key_resolver_(std::move(public_key_resolver)) {
        if (!local_private_key_) throw std::invalid_argument("local private key is required");
        if (!endpoint_resolver_) throw std::invalid_argument("endpoint resolver is required");
        if (!public_key_resolver_) throw std::invalid_argument("public key resolver is required");
    }

    AttemptResponse operator()(const JobRequest& request, const std::string& target) const {
        if (target.empty()) throw std::invalid_argument("target node is required");
        if (request.target_node_id != target)
            throw std::invalid_argument("request target does not match attempt target");

        const auto endpoint = endpoint_resolver_(target);
        if (!endpoint) return local_failure(request, target, "endpoint_unavailable");

        JobRequest signed_request = request;
        sign_job_request(signed_request, local_private_key_);

        try {
            auto socket = connect_ipv4(endpoint->host, endpoint->port);
            send_framed_json(socket.get(), job_request_envelope_json(signed_request));
            const auto response_json = recv_framed_json(socket.get());
            const auto type = wire_envelope_type(response_json);

            if (type == "JOB_RESULT") {
                const auto result = parse_job_result_envelope_json(response_json);
                if (result.request_id != request.request_id ||
                    result.responder_node_id != target ||
                    result.recipient_node_id != request.sender_node_id ||
                    result.status != "OK") {
                    return local_failure(request, target, "invalid_result_contract");
                }
                auto* public_key = public_key_resolver_(result.responder_node_id);
                if (!verify_job_result(result, public_key))
                    return local_failure(request, target, "invalid_result_signature");
                return AttemptResponse{result, std::nullopt};
            }

            if (type == "FAILURE_NOTICE") {
                const auto failure = parse_failure_notice_envelope_json(response_json);
                if (failure.request_id != request.request_id ||
                    failure.failed_node_id != target ||
                    failure.reporting_node_id.empty()) {
                    return local_failure(request, target, "invalid_failure_contract");
                }
                auto* public_key = public_key_resolver_(failure.reporting_node_id);
                if (!verify_failure_notice(failure, public_key))
                    return local_failure(request, target, "invalid_failure_signature");
                return AttemptResponse{std::nullopt, failure};
            }

            return local_failure(request, target, "unsupported_response_type");
        } catch (...) {
            return local_failure(request, target, "transport_unavailable");
        }
    }

private:
    AttemptResponse local_failure(
        const JobRequest& request,
        const std::string& target,
        const std::string& reason) const {
        FailureNotice failure{
            .protocol_version = request.protocol_version,
            .message_id = "local-failure-" + request.request_id + "-" + target,
            .request_id = request.request_id,
            .reporting_node_id = request.sender_node_id,
            .failed_node_id = target,
            .reason = reason,
            .observed_at = static_cast<std::int64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            ),
            .signature_algorithm = "Ed25519",
            .signature = "",
        };
        sign_failure_notice(failure, local_private_key_);
        return AttemptResponse{std::nullopt, failure};
    }

    EVP_PKEY* local_private_key_;
    EndpointResolver endpoint_resolver_;
    PublicKeyResolver public_key_resolver_;
};

} // namespace ma2a
