#include <iostream>
#include <string>
#include <vector>

#include <resolutive_routing/contracts.hpp>
#include <resolutive_routing/failure_adapter.hpp>
#include <resolutive_routing/reroute.hpp>
#include <resolutive_routing/router.hpp>

using namespace resolutive_routing;

static NodeSnapshot node(std::string id, double latency, double reputation) {
    NodeSnapshot n;
    n.node_id = std::move(id);
    n.organization_id = "org-1";
    n.trusted = true;
    n.available = true;
    n.compute_capacity = 100.0;
    n.current_load = 0.1;
    n.latency_ms = latency;
    n.reputation = reputation;
    n.supported_scopes = {Scope::Private};
    return n;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: routing_failover_driver <request_id> <failed_node_id> <reason> <observed_at>\n";
        return 2;
    }

    Request request;
    request.request_id = argv[1];
    request.type = RequestType::Echo;
    request.scope = Scope::Private;
    request.source_node_id = "node-a";
    request.organization_id = "org-1";

    // B is deliberately the preferred initial target; C is the valid fallback.
    std::vector<NodeSnapshot> nodes{
        node("node-b", 5.0, 1.0),
        node("node-c", 20.0, 0.90),
    };

    DeterministicRouter router;
    const auto initial = router.route(request, nodes);
    if (!initial.selected_node_id || *initial.selected_node_id != "node-b") {
        std::cerr << "unexpected initial route\n";
        return 3;
    }

    FailureNoticeView notice;
    notice.request_id = argv[1];
    notice.failed_node_id = argv[2];
    notice.reason = argv[3];
    notice.observed_at = std::stoll(argv[4]);
    notice.authenticated = true; // MA2A signature verification happens before this boundary.

    const auto failure = failure_event_from_authenticated_notice(notice);
    const auto rerouted = reroute_after_failure(router, request, nodes, initial, failure);
    if (!rerouted.selected_node_id) {
        std::cerr << "no fallback route\n";
        return 4;
    }

    std::cout << *initial.selected_node_id << "\n" << *rerouted.selected_node_id << "\n";
    return 0;
}
