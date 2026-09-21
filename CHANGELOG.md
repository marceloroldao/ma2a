# Changelog

## 0.2.0-rc1 - 2026-09-20

### Native execution and interoperability

- Add the C++20 MA2A core and cross-language Python/C++ contract tests.
- Freeze the signed job wire contract across Python and C++.
- Add framed TCP cross-language roundtrips.
- Add Ed25519-signed `FailureNotice` wire support.
- Add three-node distributed failover tests.

### Routing and resilient execution

- Integrate MA2A failover with the real `resolutive-routing` C++ decision boundary.
- Add chained B → C → D deterministic rerouting with cumulative node exclusion.
- Add the execution failover orchestrator state machine.
- Add the `ResilientExecutionEngine` API.
- Add `ResolutiveRoutingProvider` while keeping route selection owned by `resolutive-routing`.
- Add `AuthenticatedTcpAttemptExecutor` and re-sign each attempt after target selection.
- Add bounded TCP connect/read/write deadlines and SIGPIPE-safe sends.

### Adversarial and release gates

- Reject malformed envelopes, oversized frame declarations and unsupported response types.
- Reject validly signed results with incoherent request contracts.
- Reject forged `JobResult` and forged/incoherent `FailureNotice` evidence.
- Preserve authenticated local failure evidence for transport/protocol rejection.
- Distinguish route exhaustion from max-attempt exhaustion.
- Keep C++ assertions active in Release-mode CI and add a compile-time regression guard.
- Add a 30,000-request deterministic stress gate across 0, 1 and 2 failovers with zero observed divergences.
- Publish benchmark metrics as a GitHub Actions artifact.
- Pin the v0.2 release line to `resolutive-routing` commit `17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`.

## 0.1.0-rc1 - 2026-08-21

- Establish the L1/L2/L3 protocol architecture.
- Add deterministic trajectory/state synchronization and conflict handling.
- Add organizational Ed25519 PKI and challenge-response admission.
- Add replay, stale-state and negative security tests.
- Publish the archived baseline at DOI `10.5281/zenodo.22048589`.
