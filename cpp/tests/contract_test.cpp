#include <cassert>
#include <ma2a/contracts.hpp>

int main() {
    ma2a::CapabilityAdvertisement cap{
        .protocol_version = "0.2",
        .message_id = "cap-1",
        .node_id = "node-b",
        .organization_id = "org-1",
        .sequence = 1,
        .issued_at = 100,
        .expires_at = 200,
        .available = true,
        .compute_capacity = 64.0,
        .current_load = 0.25,
        .models = {"small_llm"},
        .memory_domains = {"electronics"},
        .supported_scopes = {ma2a::Scope::Organization},
    };

    assert(cap.available);
    assert(cap.expires_at > cap.issued_at);
    assert(cap.current_load >= 0.0 && cap.current_load <= 1.0);

    ma2a::JobRequest job{
        .protocol_version = "0.2",
        .request_id = "job-1",
        .sender_node_id = "node-a",
        .target_node_id = "node-b",
        .operation = "PING",
        .payload = "",
        .issued_at = 110,
        .expires_at = 120,
    };
    assert(job.target_node_id == "node-b");
    return 0;
}
