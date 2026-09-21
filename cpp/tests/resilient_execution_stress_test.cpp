#include <chrono>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <sys/resource.h>

#include <ma2a/resilient_execution.hpp>
#include <ma2a/resolutive_routing_adapter.hpp>

namespace {

resolutive_routing::NodeSnapshot node(
    std::string id,
    double latency,
    double reputation) {
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

struct Metrics {
    int failovers{};
    std::size_t iterations{};
    std::size_t divergences{};
    double wall_seconds{};
    double cpu_seconds{};
    double requests_per_second{};
    double average_microseconds{};
    long max_rss_kib{};
};

std::vector<std::string> expected_trace(int failovers) {
    if (failovers == 0) return {"node-b"};
    if (failovers == 1) return {"node-b", "node-c"};
    return {"node-b", "node-c", "node-d"};
}

Metrics run_scenario(int failovers, std::size_t iterations) {
    ma2a::ResolutiveRoutingProvider routing({
        node("node-b", 5.0, 1.00),
        node("node-c", 20.0, 0.90),
        node("node-d", 35.0, 0.80),
        node("node-e", 50.0, 0.70),
    });

    ma2a::AttemptExecutor executor =
        [failovers](const ma2a::JobRequest& request, const std::string& target) {
            ma2a::AttemptResponse response;
            const bool fail_b = failovers >= 1 && target == "node-b";
            const bool fail_c = failovers >= 2 && target == "node-c";

            if (fail_b || fail_c) {
                ma2a::FailureNotice failure;
                failure.protocol_version = request.protocol_version;
                failure.message_id = "stress-failure-" + request.request_id + "-" + target;
                failure.request_id = request.request_id;
                failure.reporting_node_id = target;
                failure.failed_node_id = target;
                failure.reason = "stress_injected_failure";
                failure.observed_at = 1800000001;
                failure.signature_algorithm = "Ed25519";
                failure.signature = "authenticated-by-stress-fixture";
                response.failure = std::move(failure);
                return response;
            }

            ma2a::JobResult result;
            result.protocol_version = request.protocol_version;
            result.message_id = "stress-result-" + request.request_id;
            result.request_id = request.request_id;
            result.responder_node_id = target;
            result.recipient_node_id = request.sender_node_id;
            result.status = "OK";
            result.payload = request.payload;
            result.completed_at = 1800000002;
            result.signature_algorithm = "Ed25519";
            result.signature = "authenticated-by-stress-fixture";
            response.result = std::move(result);
            return response;
        };

    ma2a::ResilientExecutionEngine engine(
        [&routing](const ma2a::JobRequest& request, const auto& excluded) {
            return routing(request, excluded);
        },
        executor,
        4
    );

    const auto expected = expected_trace(failovers);
    std::size_t divergences = 0;

    const auto wall_started = std::chrono::steady_clock::now();
    const std::clock_t cpu_started = std::clock();

    for (std::size_t i = 0; i < iterations; ++i) {
        ma2a::JobRequest request;
        request.protocol_version = "0.2";
        request.message_id = "stress-msg-" + std::to_string(i);
        request.request_id =
            "stress-job-" + std::to_string(failovers) + "-" + std::to_string(i);
        request.sender_node_id = "node-a";
        request.organization_id = "org-1";
        request.operation = "ECHO";
        request.payload = "stress-payload";
        request.issued_at = 1800000000;
        request.expires_at = 1800000060;

        const auto outcome = engine.execute(request);
        bool valid = outcome.state.completed && !outcome.exhausted;
        valid = valid && outcome.state.attempts.size() == expected.size();
        valid = valid && outcome.state.result.has_value();
        valid = valid && outcome.state.excluded_nodes.size() ==
            static_cast<std::size_t>(failovers);

        if (valid) {
            for (std::size_t j = 0; j < expected.size(); ++j) {
                if (outcome.state.attempts[j].node_id != expected[j]) {
                    valid = false;
                    break;
                }
                const bool final_attempt = j + 1 == expected.size();
                const std::string expected_outcome = final_attempt ? "OK" : "FAILED";
                if (outcome.state.attempts[j].outcome != expected_outcome) {
                    valid = false;
                    break;
                }
            }
        }

        if (valid && outcome.state.result->responder_node_id != expected.back())
            valid = false;
        if (valid && outcome.state.result->request_id != request.request_id)
            valid = false;

        if (!valid) ++divergences;
    }

    const std::clock_t cpu_finished = std::clock();
    const auto wall_finished = std::chrono::steady_clock::now();

    const double wall_seconds =
        std::chrono::duration<double>(wall_finished - wall_started).count();
    const double cpu_seconds =
        static_cast<double>(cpu_finished - cpu_started) / CLOCKS_PER_SEC;

    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);

    Metrics metrics;
    metrics.failovers = failovers;
    metrics.iterations = iterations;
    metrics.divergences = divergences;
    metrics.wall_seconds = wall_seconds;
    metrics.cpu_seconds = cpu_seconds;
    metrics.requests_per_second =
        wall_seconds > 0.0 ? static_cast<double>(iterations) / wall_seconds : 0.0;
    metrics.average_microseconds =
        wall_seconds > 0.0
            ? (wall_seconds * 1'000'000.0) / static_cast<double>(iterations)
            : 0.0;
    metrics.max_rss_kib = usage.ru_maxrss;
    return metrics;
}

void print_metrics(const Metrics& m, bool trailing_comma) {
    std::cout
        << "    {\"failovers\":" << m.failovers
        << ",\"iterations\":" << m.iterations
        << ",\"divergences\":" << m.divergences
        << ",\"wall_seconds\":" << std::fixed << std::setprecision(6) << m.wall_seconds
        << ",\"cpu_seconds\":" << m.cpu_seconds
        << ",\"requests_per_second\":" << std::setprecision(2) << m.requests_per_second
        << ",\"average_microseconds\":" << m.average_microseconds
        << ",\"max_rss_kib\":" << m.max_rss_kib
        << "}";
    if (trailing_comma) std::cout << ",";
    std::cout << "\n";
}

} // namespace

int main() {
    constexpr std::size_t iterations = 10'000;

    const auto zero = run_scenario(0, iterations);
    const auto one = run_scenario(1, iterations);
    const auto two = run_scenario(2, iterations);

    const std::size_t total_divergences =
        zero.divergences + one.divergences + two.divergences;

    std::cout << "{\n"
              << "  \"gate\":\"ma2a-v0.2.0-rc1-resilient-stress\",\n"
              << "  \"total_requests\":" << (iterations * 3) << ",\n"
              << "  \"total_divergences\":" << total_divergences << ",\n"
              << "  \"scenarios\":[\n";
    print_metrics(zero, true);
    print_metrics(one, true);
    print_metrics(two, false);
    std::cout << "  ]\n}\n";

    if (total_divergences != 0) {
        std::cerr << "resilient execution stress gate diverged\n";
        return 1;
    }

    return 0;
}
