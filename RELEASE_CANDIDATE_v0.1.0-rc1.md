# MA2A v0.1.0-rc1 — Release Candidate Gate

Status: PRE-RELEASE / EXPERIMENTAL

## Baseline

Public bootstrap merged to `main` at commit `a0a08cf59c5f51bf3801ef570d5b37c203c6c0f8`.

## Required gates

- [x] Public repository.
- [x] L1/L2/L3 protocol architecture documented.
- [x] Deterministic metadata conflict resolver implemented.
- [x] Replay/scope/stale/future-base convergence tests present.
- [x] Ed25519 organization/device certificate chain implemented.
- [x] HELLO / AUTH_CHALLENGE / AUTH_RESPONSE transcript model documented.
- [x] Single-use challenge replay protection implemented.
- [x] Organization status enforcement implemented.
- [x] Device/agent status enforcement implemented.
- [x] Capability enforcement implemented.
- [x] Negative tests: wrong key, wrong root, replay, expired challenge, expired certificates, tampered certificate, wrong delegation, altered nonce/transcript, revoked organization/device.
- [x] GitHub Actions green on Python 3.11.
- [x] GitHub Actions green on Python 3.12.
- [x] Interoperability test between independent Python process instances using a deterministic non-normative reference encoding.
- [x] Seeded malformed/fuzz-style tests for certificates, signatures, transcript fields and reference wire data.
- [x] Licensing text frozen for the rc1 archival candidate; production commercial agreements remain separate.
- [x] Internal technical review of key-management and revocation persistence assumptions recorded in `security/SECURITY_REVIEW_v0.1.md`.
- [ ] `resolutive-prior-art` MA2A-0001/0002/0003 pinned to the merged rc1 preparation commit/tag.
- [ ] Zenodo archival deposit and DOI after final v0.1 release freeze.

## CI evidence

The rc1 preparation branch has passed the `tests` GitHub Actions matrix on Python 3.11 and Python 3.12 after introduction of independent-process interoperability and malformed-input hardening.

## Known non-production limitations

- replay state is process-memory-only in the reference verifier;
- revocation/status persistence is supplied by the admission layer rather than implemented as a distributed status service;
- Root/organization production key custody, rotation and HSM/KMS integration are not implemented;
- the JSON delta codec is test/reference-only and is not the frozen MA2A wire encoding;
- no independent cryptographic/security audit has yet been completed.

## Release rule

`v0.1.0-rc1` is an experimental publication candidate, not a production-security designation. The rc1 tag should identify the exact public snapshot used by `resolutive-prior-art`; a final v0.1 archival DOI follows only after that cross-reference is frozen.
