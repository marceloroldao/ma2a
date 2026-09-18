#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <ma2a/contracts.hpp>

namespace ma2a {

struct ExecutionAttempt {
    std::string node_id;
    std::string outcome;
};

struct ExecutionState {
    std::string request_id;
    std::vector<ExecutionAttempt> attempts;
    std::unordered_set<std::string> excluded_nodes;
    bool completed{false};
    std::optional<JobResult> result;
};

// MA2A owns execution lifecycle state. It does not choose routes.
// A routing adapter supplies the next eligible node after observing excluded_nodes.
class ExecutionFailoverOrchestrator {
public:
    explicit ExecutionFailoverOrchestrator(std::string request_id, std::size_t max_attempts = 8)
        : state_{std::move(request_id)}, max_attempts_(max_attempts) {
        if (state_.request_id.empty()) throw std::invalid_argument("request_id is required");
        if (max_attempts_ == 0) throw std::invalid_argument("max_attempts must be positive");
    }

    [[nodiscard]] const ExecutionState& state() const noexcept { return state_; }
    [[nodiscard]] bool exhausted() const noexcept { return state_.attempts.size() >= max_attempts_; }

    void begin_attempt(const std::string& node_id) {
        ensure_active();
        if (node_id.empty()) throw std::invalid_argument("node_id is required");
        if (state_.excluded_nodes.contains(node_id)) throw std::invalid_argument("cannot retry excluded node");
        if (exhausted()) throw std::runtime_error("execution attempts exhausted");
        state_.attempts.push_back({node_id, "IN_FLIGHT"});
    }

    void accept_authenticated_failure(const FailureNotice& notice) {
        ensure_active();
        if (notice.request_id != state_.request_id) throw std::invalid_argument("failure request_id mismatch");
        if (notice.failed_node_id.empty()) throw std::invalid_argument("failed_node_id is required");
        if (notice.signature.empty()) throw std::invalid_argument("failure must be authenticated before orchestration");
        state_.excluded_nodes.insert(notice.failed_node_id);
        if (!state_.attempts.empty() && state_.attempts.back().node_id == notice.failed_node_id)
            state_.attempts.back().outcome = "FAILED";
    }

    void accept_authenticated_result(const JobResult& result) {
        ensure_active();
        if (result.request_id != state_.request_id) throw std::invalid_argument("result request_id mismatch");
        if (result.signature.empty()) throw std::invalid_argument("result must be authenticated before orchestration");
        if (result.status != "OK") throw std::invalid_argument("only successful result completes execution");
        if (!state_.attempts.empty() && state_.attempts.back().node_id == result.responder_node_id)
            state_.attempts.back().outcome = "OK";
        state_.result = result;
        state_.completed = true;
    }

private:
    void ensure_active() const {
        if (state_.completed) throw std::logic_error("execution already completed");
    }

    ExecutionState state_;
    std::size_t max_attempts_;
};

} // namespace ma2a
