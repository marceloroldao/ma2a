# MA2A — Memoria.ia Agent-to-Agent Protocol

MA2A is an experimental protocol for deterministic synchronization of resolutive memory state between agents, devices, organizational memories, and coordination services.

## Core design

MA2A separates cognition from synchronization:

- **L1 — Edge/Client:** local memory, sensors, applications, optional local LLM, private state.
- **L2 — Mesh/Transport:** authenticated peer or network transport.
- **L3 — Coordination:** routing, validation, synchronization, persistence, directory/relay functions; no LLM is required in the protocol control plane.

The protocol is designed to exchange compact trajectory/state deltas rather than requiring natural-language LLM-to-LLM messages for deterministic synchronization.

## Repository role

This repository is intended to become the canonical home of:

- MA2A protocol specifications;
- wire formats and state-delta definitions;
- deterministic conflict-resolution rules;
- organizational PKI and handshake;
- privacy-scope rules;
- reference client/server implementations;
- conformance, convergence, interoperability, replay, and security tests;
- reproducible benchmarks.

The defensive technical-disclosure registry for the wider Resolutive family is maintained separately in `marceloroldao/resolutive-prior-art`.

## Scientific/engineering status

**EXPERIMENTAL.** The current specification and implementations must not be interpreted as production security certification, universal O(1) end-to-end synchronization, or proof of superiority over other agent communication protocols.

`O(1)` claims, when used, refer only to explicitly measured known-address resolver operations. Network serialization, cryptography, persistence, routing, and conflict resolution have their own cost models.

“Zero LLM token transport” means MA2A does not require transporting natural-language LLM tokens for deterministic state synchronization. It does not mean zero network bytes.

## Planned v0.1 structure

```text
spec/
  MA2A-RFC.md
  wire-format.md
  trajectory-delta.md
  conflict-resolution.md
  privacy-scopes.md
  error-codes.md
security/
  PKI.md
  certificates.md
  handshake.md
  revocation.md
reference/
  client/
  server/
  crypto/
tests/
  conformance/
  interoperability/
  convergence/
  replay/
  security/
examples/
benchmarks/
```

## Initial trust model

```text
MA2A Official Root
        |
   Organization
        |
  Organizational Memoria.ia
    /      |       \
 device   robot    agent
```

An organization is the principal licensed/trusted network identity. Devices and agents may receive subordinate credentials under organizational policy.

## Version

Development baseline: `v0.1-dev`.
