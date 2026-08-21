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
- [ ] GitHub Actions green on Python 3.11.
- [ ] GitHub Actions green on Python 3.12.
- [ ] Interoperability test between at least two independent process instances.
- [ ] Fuzz/property tests for malformed certificates and transcript fields.
- [ ] Licensing text frozen for the archival release.
- [ ] Security review of key-management and revocation persistence assumptions.
- [ ] `resolutive-prior-art` MA2A-0001/0002/0003 pinned to the release commit/tag.
- [ ] Zenodo archival deposit and DOI after final v0.1 release freeze.

## Release rule

`v0.1.0-rc1` may be created only after CI is green and the remaining release-critical items are explicitly reviewed. This candidate does not assert production security or universal performance characteristics.
