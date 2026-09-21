# MA2A v0.2.0-rc1 — Release Candidate Gate

Status: PRE-RELEASE / EXPERIMENTAL

## Candidate baseline

The functional v0.2 line reached MA2A main commit:

`d3a5770179b000fe71a81db9c04e184f9ea550a0`

The release-preparation branch adds only versioning, documentation, metadata and an exact dependency pin.

Pinned `resolutive-routing` snapshot:

`17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`

## Required gates

### Protocol and execution

- [x] Python/C++ wire interoperability.
- [x] Signed cross-language TCP roundtrip.
- [x] Ed25519 `FailureNotice` wire contract.
- [x] Three-node distributed failover.
- [x] Real `resolutive-routing` C++ integration.
- [x] Deterministic B → C → D chained failover.
- [x] Execution failover orchestrator.
- [x] `ResilientExecutionEngine`.
- [x] `AuthenticatedTcpAttemptExecutor`.
- [x] Per-attempt re-signing after target selection.
- [x] Bounded connect/read/write deadlines.
- [x] SIGPIPE-safe failed sends.
- [x] Exact `resolutive-routing` dependency pin.

### Authentication/adversarial gate

- [x] Missing endpoint handled as locally signed failure evidence.
- [x] Connection refusal handled.
- [x] Silent peer deadline handled.
- [x] Malformed envelope rejected.
- [x] Oversized frame declaration rejected before body allocation.
- [x] Unsupported response type rejected.
- [x] Signed result with wrong request contract rejected.
- [x] Forged result signature rejected.
- [x] Valid remote failure evidence accepted.
- [x] Forged failure signature rejected.
- [x] Signed failure for the wrong attempted node rejected.
- [x] Route exhaustion distinguished from attempt exhaustion.

### Test integrity and stress

- [x] Release-mode C++ tests explicitly keep assertions enabled.
- [x] Compile-time guard fails if `NDEBUG` disables test assertions.
- [x] 17/17 C++ tests passed with assertions active in Actions run `35547692001`.
- [x] Python 3.11 and Python 3.12 passed in Actions run `35547692001`.
- [x] 30,000-request deterministic stress gate passed with zero divergences.
- [x] Benchmark metrics published as a GitHub Actions artifact.
- [x] Final release-preparation CI passes against the exact pinned routing snapshot.

## Final CI evidence

GitHub Actions run `35548055331` passed:

- Python 3.11: success
- Python 3.12: success
- C++ Release core with assertions active: success
- exact routing checkout: `17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`
- 17/17 C++ tests: success
- benchmark artifact upload: success
- 30,000-request stress divergences: 0

## Stress evidence

Final release-preparation GitHub Actions run `35548055331` (run #82), using pinned routing commit `17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`, measured:

| Failovers | Iterations | Divergences | Avg execution | Requests/s |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 10,000 | 0 | 2.37 µs | 421,579.60 |
| 1 | 10,000 | 0 | 4.16 µs | 240,441.49 |
| 2 | 10,000 | 0 | 5.61 µs | 178,322.53 |

Observed maximum RSS during that benchmark process: 3428 KiB.

These values are diagnostic and runner-specific. The release claim is **zero observed divergences in the defined deterministic gate**, not a universal throughput guarantee.

## Release metadata

- [x] Package version prepared as `0.2.0rc1`.
- [x] README updated for v0.2.
- [x] Changelog prepared.
- [x] RFC draft advanced to v0.2.
- [x] Resilient execution contract frozen.
- [x] Internal v0.2 security review recorded.
- [x] CITATION prepared without reusing the v0.1 DOI.
- [x] Zenodo metadata identifies v0.1 as the previous version.
- [x] Git tag / GitHub release `v0.2.0-rc1` created.
- [x] Zenodo archival DOI minted for the exact tagged snapshot: `10.5281/zenodo.22866124`.
- [x] Resolutive prior-art registry cross-reference updated to the exact v0.2 tag/commit.

## Archival record\n\n- GitHub tag: `v0.2.0-rc1`\n- Frozen commit: `22846a55bc9dffdec8e8cf18aa51e3ea6756fac0`\n- Zenodo DOI: `10.5281/zenodo.22866124`\n\n## Known non-production limitations

- TCP transport is not encrypted/channel-bound in the reference executor.
- The injected C++ public-key resolver is not itself bound to the organizational certificate chain.
- A production C++ execution listener with certificate admission, request replay cache and authorization is not yet shipped.
- Persistent/distributed execution replay protection is not frozen.
- Production key custody/rotation and HSM/KMS integration are not implemented.
- Distributed revocation/status propagation is not implemented.
- Production listener rate limits, quotas and backpressure are not implemented.
- No independent external security audit is claimed.

## Release rule

`v0.2.0-rc1` may be tagged only after the final CI passes against the exact pinned `resolutive-routing` snapshot.

The archived v0.1 DOI `10.5281/zenodo.22048589` must not be reused or represented as the DOI of v0.2.
