#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ma2a/authenticated_tcp_executor.hpp>
#include <ma2a/failure_notice.hpp>
#include <ma2a/job_auth.hpp>
#include <ma2a/resilient_execution.hpp>
#include <ma2a/tcp.hpp>
#include <ma2a/wire.hpp>

namespace {

std::array<unsigned char, 32> make_seed(unsigned char base) {
    std::array<unsigned char, 32> seed{};
    for (std::size_t i = 0; i < seed.size(); ++i)
        seed[i] = static_cast<unsigned char>(base + i);
    return seed;
}

ma2a::EvpPkeyPtr public_from_private(EVP_PKEY* private_key) {
    std::array<unsigned char, 32> raw{};
    std::size_t size = raw.size();
    if (EVP_PKEY_get_raw_public_key(private_key, raw.data(), &size) != 1 || size != raw.size())
        throw std::runtime_error("failed to derive public key");
    return ma2a::ed25519_public_key_from_raw(raw);
}

struct TestServer {
    int listener{-1};
    std::uint16_t port{};
    std::thread worker;
    std::shared_ptr<std::exception_ptr> error;

    TestServer() = default;
    TestServer(const TestServer&) = delete;
    TestServer& operator=(const TestServer&) = delete;
    TestServer(TestServer&&) noexcept = default;
    TestServer& operator=(TestServer&&) noexcept = default;

    void join() {
        if (worker.joinable()) worker.join();
        if (listener >= 0) {
            ::close(listener);
            listener = -1;
        }
        if (error && *error) std::rethrow_exception(*error);
    }
};

TestServer start_server(std::function<void(int)> handler) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("test socket creation failed");

    int reuse = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = 0;
    if (::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw std::runtime_error("test address setup failed");
    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        throw std::runtime_error("test bind failed");
    if (::listen(fd, 1) != 0)
        throw std::runtime_error("test listen failed");

    socklen_t len = sizeof(address);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) != 0)
        throw std::runtime_error("test getsockname failed");

    TestServer server;
    server.listener = fd;
    server.port = ntohs(address.sin_port);
    server.error = std::make_shared<std::exception_ptr>();
    const auto error = server.error;

    server.worker = std::thread([fd, handler = std::move(handler), error]() mutable {
        try {
            const int client_fd = ::accept(fd, nullptr, nullptr);
            if (client_fd < 0) throw std::runtime_error("test accept failed");
            ma2a::SocketHandle client(client_fd);
            handler(client.get());
        } catch (...) {
            *error = std::current_exception();
        }
    });

    return server;
}

std::uint16_t reserve_then_close_port() {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("test socket creation failed");

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = 0;
    if (::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
        ::close(fd);
        throw std::runtime_error("test address setup failed");
    }
    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        ::close(fd);
        throw std::runtime_error("test bind failed");
    }

    socklen_t len = sizeof(address);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) != 0) {
        ::close(fd);
        throw std::runtime_error("test getsockname failed");
    }

    const auto port = ntohs(address.sin_port);
    ::close(fd);
    return port;
}

ma2a::JobRequest base_request(const std::string& target = "node-b") {
    return ma2a::JobRequest{
        .protocol_version = "0.2",
        .message_id = "msg-adversarial-1",
        .request_id = "job-adversarial-1",
        .sender_node_id = "node-a",
        .target_node_id = target,
        .organization_id = "org-1",
        .operation = "ECHO",
        .payload = "payload",
        .issued_at = 1800000000,
        .expires_at = 1800000060,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };
}

void consume_request(int client) {
    const auto request = ma2a::recv_framed_json(client);
    assert(request.find("\"type\":\"JOB_REQUEST\"") != std::string::npos);
}

ma2a::JobResult result_for(
    const ma2a::JobRequest& request,
    const std::string& responder = "node-b") {
    return ma2a::JobResult{
        .protocol_version = request.protocol_version,
        .message_id = "msg-result-1",
        .request_id = request.request_id,
        .responder_node_id = responder,
        .recipient_node_id = request.sender_node_id,
        .status = "OK",
        .payload = request.payload,
        .completed_at = 1800000001,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };
}

ma2a::FailureNotice failure_for(
    const ma2a::JobRequest& request,
    const std::string& node = "node-b") {
    return ma2a::FailureNotice{
        .protocol_version = request.protocol_version,
        .message_id = "msg-failure-1",
        .request_id = request.request_id,
        .reporting_node_id = node,
        .failed_node_id = node,
        .reason = "remote_execution_unavailable",
        .observed_at = 1800000001,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };
}

void assert_local_failure(
    const ma2a::AttemptResponse& response,
    const ma2a::JobRequest& request,
    const std::string& target,
    const std::string& reason,
    EVP_PKEY* local_public_key) {
    assert(!response.result.has_value());
    assert(response.failure.has_value());
    assert(response.failure->request_id == request.request_id);
    assert(response.failure->reporting_node_id == request.sender_node_id);
    assert(response.failure->failed_node_id == target);
    assert(response.failure->reason == reason);
    assert(ma2a::verify_failure_notice(*response.failure, local_public_key));
}

ma2a::AuthenticatedTcpAttemptExecutor executor_for(
    EVP_PKEY* local_private_key,
    std::optional<ma2a::TcpEndpoint> endpoint,
    EVP_PKEY* target_public_key,
    std::chrono::milliseconds connect_timeout = std::chrono::milliseconds{200},
    std::chrono::milliseconds io_timeout = std::chrono::milliseconds{200}) {
    return ma2a::AuthenticatedTcpAttemptExecutor(
        local_private_key,
        [endpoint](const std::string& node_id) -> std::optional<ma2a::TcpEndpoint> {
            if (node_id == "node-b") return endpoint;
            return std::nullopt;
        },
        [target_public_key](const std::string& node_id) -> EVP_PKEY* {
            if (node_id == "node-b") return target_public_key;
            return nullptr;
        },
        connect_timeout,
        io_timeout
    );
}

} // namespace

int main() {
    auto local_private = ma2a::ed25519_private_key_from_seed(make_seed(0x01));
    auto target_private = ma2a::ed25519_private_key_from_seed(make_seed(0x21));
    auto attacker_private = ma2a::ed25519_private_key_from_seed(make_seed(0x41));
    auto local_public = public_from_private(local_private.get());
    auto target_public = public_from_private(target_private.get());

    const auto request = base_request();

    // 1. Missing endpoint is local routing/transport evidence, signed by node A.
    {
        auto executor = executor_for(
            local_private.get(), std::nullopt, target_public.get());
        const auto response = executor(request, "node-b");
        assert_local_failure(
            response, request, "node-b", "endpoint_unavailable", local_public.get());
    }

    // 2. Connection refused must terminate as transport failure.
    {
        const auto closed_port = reserve_then_close_port();
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", closed_port},
            target_public.get(),
            std::chrono::milliseconds{100},
            std::chrono::milliseconds{100});
        const auto response = executor(request, "node-b");
        assert_local_failure(
            response, request, "node-b", "transport_unavailable", local_public.get());
    }

    // 3. Malformed envelope never crosses the authentication boundary.
    {
        auto server = start_server([](int client) {
            consume_request(client);
            ma2a::send_framed_json(client, "{\"broken\":true}");
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "transport_unavailable", local_public.get());
    }

    // 4. Oversized frame declaration is rejected before a body allocation/read.
    {
        auto server = start_server([](int client) {
            consume_request(client);
            const std::vector<std::uint8_t> header{0x00, 0x10, 0x00, 0x01};
            ma2a::send_all(client, header); // 1 MiB + 1 byte
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "transport_unavailable", local_public.get());
    }

    // 5. Unknown message types are explicit protocol failures.
    {
        auto server = start_server([](int client) {
            consume_request(client);
            ma2a::send_framed_json(client, "{\"payload\":{},\"type\":\"PING\"}");
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "unsupported_response_type", local_public.get());
    }

    // 6. A correctly signed result with the wrong request_id is still invalid.
    {
        auto server = start_server([&](int client) {
            consume_request(client);
            auto result = result_for(request);
            result.request_id = "job-other";
            ma2a::sign_job_result(result, target_private.get());
            ma2a::send_framed_json(client, ma2a::job_result_envelope_json(result));
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "invalid_result_contract", local_public.get());
    }

    // 7. A structurally correct result signed by an attacker is rejected.
    {
        auto server = start_server([&](int client) {
            consume_request(client);
            auto result = result_for(request);
            ma2a::sign_job_result(result, attacker_private.get());
            ma2a::send_framed_json(client, ma2a::job_result_envelope_json(result));
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "invalid_result_signature", local_public.get());
    }

    // 8. Authenticated FailureNotice from the attempted node is accepted as remote evidence.
    {
        auto server = start_server([&](int client) {
            consume_request(client);
            auto failure = failure_for(request);
            ma2a::sign_failure_notice(failure, target_private.get());
            ma2a::send_framed_json(client, ma2a::failure_notice_envelope_json(failure));
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();

        assert(!response.result.has_value());
        assert(response.failure.has_value());
        assert(response.failure->reason == "remote_execution_unavailable");
        assert(response.failure->reporting_node_id == "node-b");
        assert(ma2a::verify_failure_notice(*response.failure, target_public.get()));
    }

    // 9. Forged FailureNotice is replaced with locally authenticated rejection evidence.
    {
        auto server = start_server([&](int client) {
            consume_request(client);
            auto failure = failure_for(request);
            ma2a::sign_failure_notice(failure, attacker_private.get());
            ma2a::send_framed_json(client, ma2a::failure_notice_envelope_json(failure));
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "invalid_failure_signature", local_public.get());
    }

    // 10. Even a valid signature cannot authorize a FailureNotice for another attempted node.
    {
        auto server = start_server([&](int client) {
            consume_request(client);
            auto failure = failure_for(request);
            failure.failed_node_id = "node-c";
            ma2a::sign_failure_notice(failure, target_private.get());
            ma2a::send_framed_json(client, ma2a::failure_notice_envelope_json(failure));
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();
        assert_local_failure(
            response, request, "node-b", "invalid_failure_contract", local_public.get());
    }

    // 11. Positive control: a valid authenticated result completes an attempt.
    {
        auto server = start_server([&](int client) {
            consume_request(client);
            auto result = result_for(request);
            ma2a::sign_job_result(result, target_private.get());
            ma2a::send_framed_json(client, ma2a::job_result_envelope_json(result));
        });
        auto executor = executor_for(
            local_private.get(),
            ma2a::TcpEndpoint{"127.0.0.1", server.port},
            target_public.get());
        const auto response = executor(request, "node-b");
        server.join();

        assert(response.result.has_value());
        assert(!response.failure.has_value());
        assert(response.result->request_id == request.request_id);
        assert(response.result->responder_node_id == "node-b");
        assert(ma2a::verify_job_result(*response.result, target_public.get()));
    }

    // 12. Route exhaustion: every known node fails and no route remains.
    {
        const std::vector<std::string> nodes{"node-b", "node-c", "node-d"};
        ma2a::RouteProvider routing =
            [nodes](const ma2a::JobRequest&, const std::unordered_set<std::string>& excluded)
                -> std::optional<std::string> {
                for (const auto& node : nodes)
                    if (!excluded.contains(node)) return node;
                return std::nullopt;
            };

        ma2a::AuthenticatedTcpAttemptExecutor unavailable_executor(
            local_private.get(),
            [](const std::string&) -> std::optional<ma2a::TcpEndpoint> {
                return std::nullopt;
            },
            [](const std::string&) -> EVP_PKEY* { return nullptr; },
            std::chrono::milliseconds{50},
            std::chrono::milliseconds{50}
        );

        auto original = base_request("");
        ma2a::ResilientExecutionEngine engine(
            routing,
            [&unavailable_executor](const ma2a::JobRequest& attempt, const std::string& target) {
                return unavailable_executor(attempt, target);
            },
            8
        );

        const auto outcome = engine.execute(original);
        assert(!outcome.state.completed);
        assert(!outcome.exhausted);
        assert(outcome.state.attempts.size() == 3);
        assert(outcome.state.excluded_nodes.size() == 3);
    }

    // 13. Attempt exhaustion is distinguishable from route exhaustion.
    {
        const std::vector<std::string> nodes{"node-b", "node-c", "node-d", "node-e"};
        ma2a::RouteProvider routing =
            [nodes](const ma2a::JobRequest&, const std::unordered_set<std::string>& excluded)
                -> std::optional<std::string> {
                for (const auto& node : nodes)
                    if (!excluded.contains(node)) return node;
                return std::nullopt;
            };

        ma2a::AuthenticatedTcpAttemptExecutor unavailable_executor(
            local_private.get(),
            [](const std::string&) -> std::optional<ma2a::TcpEndpoint> {
                return std::nullopt;
            },
            [](const std::string&) -> EVP_PKEY* { return nullptr; },
            std::chrono::milliseconds{50},
            std::chrono::milliseconds{50}
        );

        auto original = base_request("");
        ma2a::ResilientExecutionEngine engine(
            routing,
            [&unavailable_executor](const ma2a::JobRequest& attempt, const std::string& target) {
                return unavailable_executor(attempt, target);
            },
            2
        );

        const auto outcome = engine.execute(original);
        assert(!outcome.state.completed);
        assert(outcome.exhausted);
        assert(outcome.state.attempts.size() == 2);
        assert(outcome.state.excluded_nodes.size() == 2);
    }

    return 0;
}
