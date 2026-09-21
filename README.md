# MA2A — Memoria.ia Agent-to-Agent Protocol

MA2A is an experimental protocol and reference implementation for deterministic state synchronization and authenticated resilient execution between agents, devices and organizational nodes.

## Current release candidate

**Candidate:** `v0.2.0-rc1`\n\n**Zenodo DOI:** `10.5281/zenodo.22866124`

The v0.2 line adds a native C++20 execution path around the existing protocol/security baseline:

- Python ↔ C++ wire interoperability;
- Ed25519-signed `JobRequest`, `JobResult` and `FailureNotice`;
- framed TCP transport with a 1 MiB protocol frame limit;
- `ResilientExecutionEngine` with cumulative failed-node exclusion;
- direct integration with the deterministic `resolutive-routing` C++ boundary;
- reassignment B → C → D after transport/authenticated failures;
- per-attempt request re-signing after `target_node_id` is selected;
- bounded connect/read/write deadlines and SIGPIPE-safe sends;
- rejection of forged results and forged/incoherent failure evidence;
- adversarial transport/authentication gate;
- 30,000-request deterministic stress gate with zero observed divergences.

The release candidate pins `resolutive-routing` to the exact snapshot:

`17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`

Routing remains owned by `resolutive-routing`; MA2A owns the execution lifecycle, authenticated wire evidence and transport boundary.

## Archived v0.1 baseline

- **Release:** `v0.1.0-rc1`
- **Zenodo DOI:** `10.5281/zenodo.22048589`
- **Status:** experimental release candidate

That DOI identifies only the archived v0.1 snapshot. It must not be presented as the DOI for v0.2.

## Architecture

MA2A separates cognition from synchronization/execution:

- **L1 — Edge/Client:** local memory, sensors, applications, optional local LLM and private state.
- **L2 — Transport/Mesh:** authenticated exchange between peers.
- **L3 — Coordination:** routing integration, validation, synchronization, persistence and relay functions.

LLMs are optional application components and are not required in the deterministic protocol control path.

### Responsibility boundary

```text
Memoria.ia
  -> memory/state semantics

resolutive-routing
  -> admissibility, scoring, deterministic route selection and rerouting

MA2A
  -> identity boundary, signed wire messages, transport,
     execution lifecycle, failure evidence and result delivery
```

## Trust model

```text
MA2A Official Root
        |
   Organization
        |
  Organizational Memoria.ia
    /      |       \
 device   robot    agent
```

The initial signature primitive is Ed25519.

## Protocol limits

See `PROTOCOL_LIMITS.md`.

The v0.2 execution wire is frozen in `spec/resilient-execution-v0.2.md`.

## Validation

The v0.2 candidate currently includes:

- Python test matrix on 3.11 and 3.12;
- C++ Release-mode tests with assertions explicitly kept active;
- Python/C++ signed TCP roundtrip;
- real `resolutive-routing` failover integration;
- authenticated TCP resilient execution;
- adversarial malformed/forged input tests;
- deterministic stress testing across 0, 1 and 2 failovers.

Benchmark values are diagnostic and environment-specific; they are not universal performance claims.

## Security status

**EXPERIMENTAL / RELEASE CANDIDATE.**

This is not a production-security certification. In particular, the current C++ resilient executor does not itself provide:

- encrypted transport/channel confidentiality;
- automatic certificate-chain binding inside the injected public-key resolver;
- a production C++ execution listener with persistent replay protection;
- distributed revocation/status propagation;
- HSM/KMS-backed production key custody;
- an independent external security audit.

See `security/SECURITY_REVIEW_v0.2.md`.

## Scientific/engineering claims

“Zero LLM token transport” means natural-language LLM token streams are not required for deterministic MA2A synchronization/execution control. It does not mean zero network bytes.

Any O(1) claim must name the exact local operation measured. End-to-end routing, networking, cryptography and persistence have their own cost models.

## Version

Current prepared candidate: `v0.2.0-rc1`.

Latest archived DOI-bearing baseline: `v0.1.0-rc1`.
