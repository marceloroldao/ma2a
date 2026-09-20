#pragma once
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <ma2a/contracts.hpp>
#include <resolutive_routing/contracts.hpp>
#include <resolutive_routing/router.hpp>

namespace ma2a {

class ResolutiveRoutingProvider {
public:
    explicit ResolutiveRoutingProvider(std::vector<resolutive_routing::NodeSnapshot> nodes)
        : nodes_(std::move(nodes)) {}

    std::optional<std::string> operator()(
        const JobRequest& job,
        const std::unordered_set<std::string>& excluded) const {
        resolutive_routing::Request request;
        request.request_id = job.request_id;
        request.type = resolutive_routing::RequestType::Echo;
        request.scope = resolutive_routing::Scope::Private;
        request.source_node_id = job.sender_node_id;
        request.organization_id = job.organization_id;

        std::vector<resolutive_routing::NodeSnapshot> eligible;
        eligible.reserve(nodes_.size());
        for (const auto& node : nodes_) {
            if (!excluded.contains(node.node_id)) eligible.push_back(node);
        }

        const auto decision = router_.route(request, eligible);
        return decision.selected_node_id;
    }

private:
    std::vector<resolutive_routing::NodeSnapshot> nodes_;
    resolutive_routing::DeterministicRouter router_;
};

} // namespace ma2a
