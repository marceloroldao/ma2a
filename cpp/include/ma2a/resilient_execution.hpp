#pragma once
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <ma2a/contracts.hpp>
#include <ma2a/execution_orchestrator.hpp>

namespace ma2a {

struct AttemptResponse {
    std::optional<JobResult> result;
    std::optional<FailureNotice> failure;
};

using RouteProvider = std::function<std::optional<std::string>(
    const JobRequest&, const std::unordered_set<std::string>&)>;

using AttemptExecutor = std::function<AttemptResponse(
    const JobRequest&, const std::string&)>;

struct ResilientExecutionOutcome {
    ExecutionState state;
    bool exhausted{false};
};

class ResilientExecutionEngine {
public:
    ResilientExecutionEngine(RouteProvider route_provider,
                             AttemptExecutor attempt_executor,
                             std::size_t max_attempts = 8)
        : route_provider_(std::move(route_provider)),
          attempt_executor_(std::move(attempt_executor)),
          max_attempts_(max_attempts) {
        if (!route_provider_) throw std::invalid_argument("route provider is required");
        if (!attempt_executor_) throw std::invalid_argument("attempt executor is required");
        if (max_attempts_ == 0) throw std::invalid_argument("max_attempts must be positive");
    }

    ResilientExecutionOutcome execute(const JobRequest& original) const {
        if (original.request_id.empty()) throw std::invalid_argument("request_id is required");

        ExecutionFailoverOrchestrator lifecycle(original.request_id, max_attempts_);

        while (!lifecycle.state().completed && !lifecycle.exhausted()) {
            const auto target = route_provider_(original, lifecycle.state().excluded_nodes);
            if (!target) break;
            if (lifecycle.state().excluded_nodes.contains(*target))
                throw std::logic_error("routing returned an excluded node");

            lifecycle.begin_attempt(*target);
            JobRequest attempt = original;
            attempt.target_node_id = *target;

            const auto response = attempt_executor_(attempt, *target);
            if (response.result && response.failure)
                throw std::logic_error("attempt cannot return result and failure together");
            if (!response.result && !response.failure)
                throw std::logic_error("attempt returned no terminal evidence");

            if (response.result) {
                lifecycle.accept_authenticated_result(*response.result);
            } else {
                if (response.failure->failed_node_id != *target)
                    throw std::invalid_argument("failure does not identify attempted node");
                lifecycle.accept_authenticated_failure(*response.failure);
            }
        }

        return {lifecycle.state(), !lifecycle.state().completed};
    }

private:
    RouteProvider route_provider_;
    AttemptExecutor attempt_executor_;
    std::size_t max_attempts_;
};

} // namespace ma2a
