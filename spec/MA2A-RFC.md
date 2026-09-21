# MA2A Protocol — RFC Draft v0.2

Status: EXPERIMENTAL RELEASE-CANDIDATE DRAFT

## 1. Scope

MA2A defines deterministic state synchronization and authenticated execution exchange between resolutive agents and devices without requiring natural-language LLM-to-LLM transport in the protocol control path.

## 2. Architecture

- L1: edge/client cognition and local/private memory.
- L2: authenticated transport or mesh.
- L3: coordination, validation, routing integration, persistence and relay.

L3 does not require an LLM for protocol operation.

## 3. Core synchronization objects

A MA2A synchronization implementation MUST support a versioned state-delta object containing at least:

```text
protocol_version
message_id
sender_id
organization_id
trajectory_address
base_version
new_version
operation
scope
payload_or_reference
integrity_digest
logical_clock
signature
```

## 4. Privacy scopes

Minimum scopes:

- `private`: device-local by default;
- `user`: restricted to an authorized user domain;
- `shared`: explicitly shareable within an authorized peer/organization set;
- `global`: eligible for wider publication/replication subject to policy.

No intermediate or central node may silently broaden a scope.

## 5. Synchronization contract

A receiver MUST authenticate the sender, validate message integrity, enforce scope, reject replay/stale mutations, then apply a deterministic versioned conflict rule.

## 6. Identity

The initial trust hierarchy is:

```text
MA2A Official Root
  -> Organization Certificate
     -> Organization Memoria.ia / delegated device or agent credential
```

The initial signature primitive is Ed25519. Algorithm agility is required for future revisions.

## 7. Admission handshake

The initial session handshake is challenge-response:

```text
CLIENT -> HELLO
SERVER -> AUTH_CHALLENGE
CLIENT -> AUTH_RESPONSE
SERVER -> AUTH_OK | AUTH_DENIED
```

See `security/handshake.md`.

## 8. Determinism boundary

The protocol requires deterministic outcomes for identical accepted inputs under the same protocol/policy version.

An LLM MAY be used by an application before or after synchronization/execution, but MUST NOT be the sole normative conflict-resolution or routing authority.

## 9. Routing boundary

MA2A does not own route scoring or route selection.

The v0.2 reference integrates with `resolutive-routing`, which owns:

- route admissibility;
- deterministic scoring;
- candidate selection;
- rerouting decisions.

MA2A owns authenticated transport attempts and execution lifecycle state.

The release-candidate routing snapshot is pinned by commit SHA; it is not taken from a floating `main`.

## 10. Resilient execution extension

The v0.2 execution extension introduces:

- `JobRequest`;
- `JobResult`;
- `FailureNotice`;
- per-attempt target-bound signing;
- authenticated result/failure acceptance;
- cumulative failed-node exclusion;
- route exhaustion and max-attempt terminal states;
- bounded TCP attempt deadlines.

The normative reference contract is `spec/resilient-execution-v0.2.md`.

## 11. Performance terminology

“Zero LLM token transport” means no natural-language LLM-token stream is required for deterministic MA2A state synchronization or execution control. It does not mean zero bytes.

Any O(1) claim MUST name the exact local operation being measured. End-to-end network synchronization, routing, cryptography, execution and persistence are not asserted to be universally O(1).

Benchmark results in the repository are environment-specific diagnostics, not universal performance claims.

## 12. Security status

This v0.2 release-candidate draft is not a security certification.

The reference execution layer uses signed evidence and adversarial regression tests, but production deployments still require channel security, PKI-bound key resolution, persistent replay protection, certificate lifecycle management, tenant isolation, resource limits, secure key custody and independent security review.

See `security/SECURITY_REVIEW_v0.2.md`.
