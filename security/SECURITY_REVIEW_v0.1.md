# MA2A v0.1 Security Review — Key Management and Revocation Assumptions

Status: internal technical review for experimental release candidate. This is not an independent security audit.

Date: 2026-08-21

## 1. What is already enforced by the reference implementation

The v0.1 reference path enforces:

- Ed25519 signatures for the initial certificate/signature primitive;
- Root -> Organization -> Device/Agent certificate verification;
- certificate validity intervals;
- strict certificate version/signature-algorithm checks;
- organization identity binding to the challenge;
- device/agent identity binding to the challenge;
- organization and delegated-device ACTIVE status checks;
- delegated capability checks;
- canonical challenge transcript signing;
- challenge expiration;
- single-use challenge replay rejection;
- fail-closed malformed Base64/signature/certificate handling;
- wrong-root and wrong-delegation rejection.

## 2. Root key management

The repository contains no production Root private key and must never contain one.

Production assumptions that remain outside the reference implementation:

- Root key generation must occur in a controlled environment;
- production Root signing should use an HSM, KMS, offline signing process, or equivalent hardened key custody;
- routine online operations should not require the Root private key;
- organization issuance should support auditable serial numbers and issuance records;
- Root rotation/cross-signing/recovery procedures must be defined before production use.

**v0.1 status:** acceptable for an experimental protocol release; not production-ready key management.

## 3. Organization key management

Organizations are expected to control an organizational signing authority that can issue delegated device/agent credentials under policy constraints.

Open production questions:

- secure storage of organization private keys;
- whether an organization uses an offline CA plus online intermediate;
- maximum delegation depth;
- capability narrowing rules;
- key rotation and overlap periods;
- lost/compromised device recovery;
- audit logging requirements.

For v0.1, the reference implementation supports one Root -> Organization -> Device/Agent delegation level. It does not claim arbitrary PKI-chain support.

## 4. Replay-state persistence

`HandshakeVerifier` currently stores consumed challenge IDs in process memory.

This means replay protection is correct only within the lifetime/state of that verifier instance. A process restart or a horizontally scaled deployment without shared replay state can lose knowledge of consumed challenges.

Production implementations therefore require one of:

- shared TTL-backed challenge storage;
- durable replay cache;
- deterministic stateless token construction with server-secret binding plus anti-reuse policy;
- or another reviewed equivalent.

**Finding:** in-memory replay state is sufficient for the reference implementation and tests, but MUST NOT be represented as production distributed replay protection.

## 5. Revocation persistence

The reference verifier receives `organization_status` and `device_status` from the admission layer. It does not itself persist a CRL/status database.

A production MA2A Official deployment requires an authoritative status source with at least:

- ACTIVE;
- SUSPENDED;
- EXPIRED;
- REVOKED;
- effective timestamp;
- reason/audit record where appropriate;
- deterministic propagation/cache policy.

Distributed L3 nodes must not indefinitely cache ACTIVE state after a revocation event.

**Finding:** the protocol boundary is specified and tested, but persistent revocation distribution remains an infrastructure responsibility.

## 6. Clock assumptions

Certificate and challenge validity currently depends on wall-clock timestamps.

Production deployments need:

- bounded clock-skew policy;
- secure/reliable time source expectations;
- behavior when local time is outside tolerated skew;
- careful distinction between wall-clock expiry and logical clocks used for trajectory ordering.

Logical trajectory clocks must not be substituted for certificate-validity time without a separate design.

## 7. Session binding after AUTH_OK

The current reference verifies authentication but does not yet derive a session key or cryptographically bind every subsequent delta to a negotiated transport session.

Production options include:

- TLS 1.3 plus authenticated MA2A identity binding;
- signed deltas independent of transport;
- channel binding/exporter mechanisms;
- mutually authenticated transport profiles.

The v0.1 protocol must not imply that Ed25519 challenge authentication replaces transport confidentiality.

## 8. Denial-of-service and parser bounds

The reference trajectory-delta JSON test codec now applies a 64 KiB bound and strict field set. Production binary codecs require their own explicit bounds for:

- certificate chain size;
- capability count;
- identifier length;
- payload/reference length;
- nesting/delegation depth;
- handshake rate;
- outstanding challenges per identity/source.

## 9. Metadata privacy

Even when payloads remain local/private, organization IDs, device IDs, trajectory addresses, timing, scope and routing metadata can disclose relationships or behavior.

Production privacy design should consider pseudonymous identifiers, selective disclosure, minimum logging, retention limits and traffic-analysis exposure.

## 10. Algorithm agility

Ed25519 is the v0.1 primitive, not a permanent protocol invariant. Certificates carry an algorithm identifier and later protocol versions must define safe negotiation/migration instead of silently accepting arbitrary algorithms.

Downgrade prevention is required when more than one algorithm is supported.

## 11. Release assessment

### Suitable for v0.1.0-rc1 experimental publication

- architecture and trust hierarchy;
- Ed25519 reference issuance/verification;
- challenge-response transcript;
- deterministic rejection tests;
- organization/device status boundary;
- documented operational limitations.

### Not suitable to claim yet

- production-grade PKI operation;
- durable distributed replay protection;
- production revocation propagation SLA;
- HSM-backed Root operations;
- formal cryptographic proof;
- independent penetration test/security audit;
- safety-critical authentication suitability.

## 12. Next security milestones

1. define persistent challenge/status-store interfaces;
2. add certificate/key rotation profiles;
3. define TLS/channel-binding profile;
4. fuzz final chosen wire codec and certificate parser;
5. perform independent security review before production `v1.0` designation.
