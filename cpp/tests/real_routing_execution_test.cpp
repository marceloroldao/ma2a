#include <cassert>
#include <string>
#include <vector>

#include <ma2a/resilient_execution.hpp>
#include <ma2a/resolutive_routing_adapter.hpp>

using namespace ma2a;

static resolutive_routing::NodeSnapshot node(std::string id, double latency, double reputation) {
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

int main() {
    ResolutiveRoutingProvider routing({
        node("node-b", 5.0, 1.0),
        node("node-c", 20.0, 0.90),
        node("node-d", 35.0, 0.80)
    });

    AttemptExecutor executor = [](const JobRequest& request, const std::string& target) {
        AttemptResponse response;
        if (target == "node-b" || target == "node-c") {
            FailureNotice failure;
            failure.request_id = request.request_id;
            failure.reporting_node_id = target;
            failure.failed_node_id = target;
            failure.reason = "execution_unavailable";
            failure.signature = "verified-before-engine";
            response.failure = failure;
            return response;
        }
        JobResult result;
        result.request_id = request.request_id;
        result.responder_node_id = target;
        result.status = "OK";
        result.payload = request.payload;
        result.signature = "verified-before-engine";
        response.result = result;
        return response;
    };

    JobRequest job;
    job.request_id = "job-real-routing-1";
    job.sender_node_id = "node-a";
    job.organization_id = "org-1";
    job.operation = "ECHO";
    job.payload = "real-routing";

    ResilientExecutionEngine engine(
        [&routing](const JobRequest& request, const auto& excluded) {
            return routing(request, excluded);
        },
        executor,
        4
    );

    const auto outcome = engine.execute(job);
    assert(outcome.state.completed);
    assert(outcome.state.attempts.size() == 3);
    assert(outcome.state.attempts[0].node_id == "node-b");
    assert(outcome.state.attempts[1].node_id == "node-c");
    assert(outcome.state.attempts[2].node_id == "node-d");
    assert(outcome.state.result->payload == "real-routing");
    return 0;
}
