#include <cassert>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <ma2a/resilient_execution.hpp>

using namespace ma2a;

static FailureNotice failure(const JobRequest& request, const std::string& node) {
    FailureNotice f;
    f.request_id = request.request_id;
    f.failed_node_id = node;
    f.reporting_node_id = node;
    f.reason = "execution_unavailable";
    f.signature = "verified-failure-signature";
    return f;
}

int main() {
    std::vector<std::string> route_trace;
    RouteProvider routing = [&](const JobRequest&, const std::unordered_set<std::string>& excluded)
        -> std::optional<std::string> {
        for (const auto* node : {"node-b", "node-c", "node-d"}) {
            if (!excluded.contains(node)) {
                route_trace.emplace_back(node);
                return std::string(node);
            }
        }
        return std::nullopt;
    };

    AttemptExecutor transport = [](const JobRequest& request, const std::string& node) {
        AttemptResponse response;
        if (node == "node-b" || node == "node-c") {
            response.failure = failure(request, node);
            return response;
        }
        JobResult result;
        result.request_id = request.request_id;
        result.responder_node_id = node;
        result.status = "OK";
        result.payload = request.payload;
        result.signature = "verified-result-signature";
        response.result = result;
        return response;
    };

    JobRequest request;
    request.request_id = "job-resilient-1";
    request.sender_node_id = "node-a";
    request.operation = "ECHO";
    request.payload = "payload";

    ResilientExecutionEngine engine(routing, transport, 4);
    const auto outcome = engine.execute(request);

    assert(outcome.state.completed);
    assert(!outcome.exhausted);
    assert(outcome.state.attempts.size() == 3);
    assert(outcome.state.attempts[0].node_id == "node-b");
    assert(outcome.state.attempts[1].node_id == "node-c");
    assert(outcome.state.attempts[2].node_id == "node-d");
    assert(outcome.state.excluded_nodes.contains("node-b"));
    assert(outcome.state.excluded_nodes.contains("node-c"));
    assert(outcome.state.result->responder_node_id == "node-d");
    assert(route_trace == std::vector<std::string>({"node-b", "node-c", "node-d"}));

    RouteProvider no_route = [](const JobRequest&, const std::unordered_set<std::string>&)
        -> std::optional<std::string> { return std::nullopt; };
    ResilientExecutionEngine unavailable(no_route, transport, 2);
    const auto empty = unavailable.execute(request);
    assert(!empty.state.completed);
    assert(empty.exhausted);
    assert(empty.state.attempts.empty());

    RouteProvider bad_route = [](const JobRequest&, const std::unordered_set<std::string>& excluded)
        -> std::optional<std::string> {
        return excluded.empty() ? std::optional<std::string>("node-b")
                                : std::optional<std::string>("node-b");
    };
    ResilientExecutionEngine invalid(bad_route, transport, 2);
    bool rejected = false;
    try { (void)invalid.execute(request); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);

    return 0;
}
