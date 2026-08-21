# MA2A Protocol — RFC Draft v0.1

Status: EXPERIMENTAL DRAFT

## 1. Scope

MA2A defines deterministic state synchronization between resolutive agents and devices without requiring natural-language LLM-to-LLM transport in the synchronization path.

## 2. Architecture

- L1: edge/client cognition and local/private memory.
- L2: authenticated transport or mesh.
- L3: coordination, validation, routing, persistence and relay.

L3 does not require an LLM for protocol operation.

## 3. Core objects

A MA2A implementation MUST support a versioned state-delta object containing at least:

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

The protocol requires deterministic outcomes for identical accepted inputs under the same protocol/policy version. An LLM MAY be used by an application before or after synchronization, but MUST NOT be the sole normative conflict-resolution authority.

## 9. Performance terminology

“Zero LLM token transport” means no natural-language LLM-token stream is required for deterministic MA2A state synchronization. It does not mean zero bytes.

Any O(1) claim MUST name the exact local operation being measured. End-to-end network synchronization is not asserted to be universally O(1).

## 10. Security status

This v0.1 draft is not a security certification. Production deployments require threat modeling, key management, replay protection, certificate lifecycle management, tenant isolation and independent security review.
