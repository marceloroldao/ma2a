# MA2A v0.2.0-rc1

Status: **PRE-RELEASE / EXPERIMENTAL**

This release candidate freezes the authenticated resilient-execution line at:

`22846a55bc9dffdec8e8cf18aa51e3ea6756fac0`

It pins `resolutive-routing` to:

`17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`

## Highlights

- C++20 MA2A execution core.
- Python/C++ signed wire interoperability.
- Ed25519-signed `JobRequest`, `JobResult` and `FailureNotice`.
- Real `resolutive-routing` integration.
- Deterministic B → C → D failover with cumulative exclusions.
- Per-attempt re-signing after target selection.
- Bounded TCP connect/read/write deadlines.
- SIGPIPE-safe failed sends.
- Rejection of malformed, oversized, forged and contract-incoherent responses.
- Release-mode C++ tests with assertions explicitly active.
- 17/17 C++ tests green on the frozen candidate.
- Python 3.11 and Python 3.12 green.
- 30,000-request deterministic stress gate with zero observed divergences.

## Security status

This is not a production-security certification.

Known open production requirements include encrypted/channel-bound transport, PKI-bound key resolution in the C++ executor, persistent replay protection, production execution listener hardening, distributed revocation/status propagation, secure production key custody and independent external audit.

See:

- `spec/resilient-execution-v0.2.md`
- `security/SECURITY_REVIEW_v0.2.md`
- `RELEASE_CANDIDATE_v0.2.0-rc1.md`

## Archival status

The historical v0.1 DOI `10.5281/zenodo.22048589` applies only to the archived v0.1 snapshot and is **not** the DOI of this v0.2 release candidate.

A new Zenodo DOI must be recorded only after Zenodo archives this exact tagged snapshot.
