#include <cassert>
#include <stdexcept>
#include <string>

#include <ma2a/execution_orchestrator.hpp>

using namespace ma2a;

static FailureNotice failure(const std::string& request_id, const std::string& node) {
    FailureNotice n;
    n.request_id = request_id;
    n.failed_node_id = node;
    n.reporting_node_id = node;
    n.reason = "execution_unavailable";
    n.signature = "authenticated-signature";
    return n;
}

int main() {
    ExecutionFailoverOrchestrator orchestrator("job-1", 3);

    orchestrator.begin_attempt("node-b");
    orchestrator.accept_authenticated_failure(failure("job-1", "node-b"));
    assert(orchestrator.state().excluded_nodes.contains("node-b"));
    assert(orchestrator.state().attempts.back().outcome == "FAILED");

    bool rejected = false;
    try { orchestrator.begin_attempt("node-b"); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);

    orchestrator.begin_attempt("node-c");
    orchestrator.accept_authenticated_failure(failure("job-1", "node-c"));
    assert(orchestrator.state().excluded_nodes.contains("node-c"));

    orchestrator.begin_attempt("node-d");
    JobResult result;
    result.request_id = "job-1";
    result.responder_node_id = "node-d";
    result.status = "OK";
    result.payload = "done";
    result.signature = "authenticated-result-signature";
    orchestrator.accept_authenticated_result(result);

    assert(orchestrator.state().completed);
    assert(orchestrator.state().result.has_value());
    assert(orchestrator.state().result->payload == "done");
    assert(orchestrator.state().attempts.size() == 3);
    assert(orchestrator.state().attempts[0].node_id == "node-b");
    assert(orchestrator.state().attempts[1].node_id == "node-c");
    assert(orchestrator.state().attempts[2].node_id == "node-d");
    assert(orchestrator.state().attempts[2].outcome == "OK");

    bool closed = false;
    try { orchestrator.begin_attempt("node-e"); }
    catch (const std::logic_error&) { closed = true; }
    assert(closed);

    ExecutionFailoverOrchestrator limited("job-2", 1);
    limited.begin_attempt("node-x");
    limited.accept_authenticated_failure(failure("job-2", "node-x"));
    bool exhausted = false;
    try { limited.begin_attempt("node-y"); }
    catch (const std::runtime_error&) { exhausted = true; }
    assert(exhausted);

    return 0;
}
