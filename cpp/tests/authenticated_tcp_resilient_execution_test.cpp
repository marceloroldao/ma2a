#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ma2a/authenticated_tcp_executor.hpp>
#include <ma2a/job_auth.hpp>
#include <ma2a/resilient_execution.hpp>
#include <ma2a/resolutive_routing_adapter.hpp>
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

    void join() {
        if (worker.joinable()) worker.join();
        if (listener >= 0) {
            ::close(listener);
            listener = -1;
        }
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
    server.worker = std::thread([fd, handler = std::move(handler)]() mutable {
        const int client_fd = ::accept(fd, nullptr, nullptr);
        if (client_fd < 0) std::abort();
        ma2a::SocketHandle client(client_fd);
        try {
            handler(client.get());
        } catch (...) {
            std::abort();
        }
    });
    return server;
}

resolutive_routing::NodeSnapshot node(std::string id, double latency, double reputation) {
    resolutive_routing::NodeSnapshot n;
    n.node_id = std::move(id);
    n.organization_id = "org-1";
    n.trusted = true;
    n.available = true;
    n.compute_capacity = 100.0;
    n.current_load = 0.1;
    n.latency_ms = latency;
    n.reputation = reputation;
    n.supported_scopes = {resolutive_routing::Scope::Private};
    return n;
}

std::string signed_attempt_envelope(
    const ma2a::JobRequest& original,
    const std::string& target,
    EVP_PKEY* private_key) {
    auto attempt = original;
    attempt.target_node_id = target;
    ma2a::sign_job_request(attempt, private_key);
    return ma2a::job_request_envelope_json(attempt);
}

} // namespace

int main() {
    auto a_private = ma2a::ed25519_private_key_from_seed(make_seed(0x01));
    auto b_private = ma2a::ed25519_private_key_from_seed(make_seed(0x21));
    auto c_private = ma2a::ed25519_private_key_from_seed(make_seed(0x41));
    auto d_private = ma2a::ed25519_private_key_from_seed(make_seed(0x61));
    auto attacker_private = ma2a::ed25519_private_key_from_seed(make_seed(0x81));

    auto b_public = public_from_private(b_private.get());
    auto c_public = public_from_private(c_private.get());
    auto d_public = public_from_private(d_private.get());

    ma2a::JobRequest job{
        .protocol_version = "0.2",
        .message_id = "msg-auth-tcp-1",
        .request_id = "job-auth-tcp-1",
        .sender_node_id = "node-a",
        .target_node_id = "",
        .organization_id = "org-1",
        .operation = "ECHO",
        .payload = "authenticated-real-routing",
        .issued_at = 1800000000,
        .expires_at = 1800000060,
        .signature_algorithm = "Ed25519",
        .signature = "",
    };

    const auto expected_b = signed_attempt_envelope(job, "node-b", a_private.get());
    const auto expected_c = signed_attempt_envelope(job, "node-c", a_private.get());
    const auto expected_d = signed_attempt_envelope(job, "node-d", a_private.get());

    auto server_b = start_server([expected_b](int client) {
        assert(ma2a::recv_framed_json(client) == expected_b);
        // Close without a response: the executor must create a locally signed
        // transport failure and the engine must continue to the next route.
    });

    auto server_c = start_server([expected_c, &attacker_private](int client) {
        assert(ma2a::recv_framed_json(client) == expected_c);
        ma2a::JobResult result{
            .protocol_version = "0.2",
            .message_id = "msg-result-c",
            .request_id = "job-auth-tcp-1",
            .responder_node_id = "node-c",
            .recipient_node_id = "node-a",
            .status = "OK",
            .payload = "authenticated-real-routing",
            .completed_at = 1800000001,
            .signature_algorithm = "Ed25519",
            .signature = "",
        };
        ma2a::sign_job_result(result, attacker_private.get());
        ma2a::send_framed_json(client, ma2a::job_result_envelope_json(result));
    });

    auto server_d = start_server([expected_d, &d_private](int client) {
        assert(ma2a::recv_framed_json(client) == expected_d);
        ma2a::JobResult result{
            .protocol_version = "0.2",
            .message_id = "msg-result-d",
            .request_id = "job-auth-tcp-1",
            .responder_node_id = "node-d",
            .recipient_node_id = "node-a",
            .status = "OK",
            .payload = "authenticated-real-routing",
            .completed_at = 1800000002,
            .signature_algorithm = "Ed25519",
            .signature = "",
        };
        ma2a::sign_job_result(result, d_private.get());
        ma2a::send_framed_json(client, ma2a::job_result_envelope_json(result));
    });

    ma2a::ResolutiveRoutingProvider routing({
        node("node-b", 5.0, 1.0),
        node("node-c", 20.0, 0.90),
        node("node-d", 35.0, 0.80)
    });

    ma2a::EndpointResolver endpoint_resolver =
        [&](const std::string& node_id) -> std::optional<ma2a::TcpEndpoint> {
            if (node_id == "node-b") return ma2a::TcpEndpoint{"127.0.0.1", server_b.port};
            if (node_id == "node-c") return ma2a::TcpEndpoint{"127.0.0.1", server_c.port};
            if (node_id == "node-d") return ma2a::TcpEndpoint{"127.0.0.1", server_d.port};
            return std::nullopt;
        };

    ma2a::PublicKeyResolver public_key_resolver =
        [&](const std::string& node_id) -> EVP_PKEY* {
            if (node_id == "node-b") return b_public.get();
            if (node_id == "node-c") return c_public.get();
            if (node_id == "node-d") return d_public.get();
            return nullptr;
        };

    ma2a::AuthenticatedTcpAttemptExecutor tcp_executor(
        a_private.get(),
        endpoint_resolver,
        public_key_resolver
    );

    ma2a::ResilientExecutionEngine engine(
        [&routing](const ma2a::JobRequest& request, const auto& excluded) {
            return routing(request, excluded);
        },
        [&tcp_executor](const ma2a::JobRequest& request, const std::string& target) {
            return tcp_executor(request, target);
        },
        4
    );

    const auto outcome = engine.execute(job);

    server_b.join();
    server_c.join();
    server_d.join();

    assert(outcome.state.completed);
    assert(!outcome.exhausted);
    assert(outcome.state.attempts.size() == 3);
    assert(outcome.state.attempts[0].node_id == "node-b");
    assert(outcome.state.attempts[0].outcome == "FAILED");
    assert(outcome.state.attempts[1].node_id == "node-c");
    assert(outcome.state.attempts[1].outcome == "FAILED");
    assert(outcome.state.attempts[2].node_id == "node-d");
    assert(outcome.state.attempts[2].outcome == "OK");
    assert(outcome.state.excluded_nodes.contains("node-b"));
    assert(outcome.state.excluded_nodes.contains("node-c"));
    assert(outcome.state.result.has_value());
    assert(outcome.state.result->responder_node_id == "node-d");
    assert(outcome.state.result->payload == job.payload);

    return 0;
}
