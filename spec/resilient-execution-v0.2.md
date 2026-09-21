# MA2A Resilient Execution Contract v0.2

Status: EXPERIMENTAL RELEASE-CANDIDATE CONTRACT

This document freezes the MA2A v0.2 reference boundary for authenticated job execution and deterministic failover.

## 1. Responsibility boundary

MA2A owns:

- authenticated job/result/failure wire messages;
- transport attempts;
- execution lifecycle state;
- failed-node exclusion for the current execution;
- acceptance/rejection of authenticated terminal evidence.

`resolutive-routing` owns:

- admissibility;
- deterministic candidate scoring;
- next-node selection;
- rerouting decisions.

Memoria.ia owns memory/state semantics.

The MA2A v0.2 release candidate pins the routing dependency to:

`17bf787d92589ad398bf9f65c1eecbbbbde8f6b1`

## 2. Framing

Reference TCP framing is:

```text
4-byte unsigned big-endian body length
+
UTF-8 JSON body
```

Normative limits:

- zero-length frames are invalid;
- maximum body length is 1 MiB;
- the body limit is checked before allocating/reading the announced body.

See `PROTOCOL_LIMITS.md`.

## 3. Envelopes

The reference execution envelope has exactly:

```json
{"payload":{...},"type":"JOB_REQUEST"}
```

or:

```json
{"payload":{...},"type":"JOB_RESULT"}
```

or:

```json
{"payload":{...},"type":"FAILURE_NOTICE"}
```

Unknown response types are not accepted as execution evidence.

## 4. JobRequest

Fields:

```text
protocol_version
message_id
request_id
sender_node_id
target_node_id
organization_id
operation
payload
issued_at
expires_at
signature_algorithm
signature
```

The v0.2 reference signature algorithm is Ed25519.

The canonical signed payload omits `signature` and uses canonical UTF-8 JSON. Canonical key order is:

```text
expires_at
issued_at
message_id
operation
organization_id
payload
protocol_version
request_id
sender_node_id
signature_algorithm
target_node_id
```

### Per-attempt signing rule

A route is selected before the final wire request is signed.

Therefore every execution attempt MUST:

1. obtain the selected target from the routing boundary;
2. set `target_node_id`;
3. canonicalize the resulting request;
4. sign that exact request;
5. send the signed attempt.

A signature from a previous target MUST NOT be reused after rerouting.

## 5. JobResult

Fields:

```text
protocol_version
message_id
request_id
responder_node_id
recipient_node_id
status
payload
completed_at
signature_algorithm
signature
```

Canonical key order:

```text
completed_at
message_id
payload
protocol_version
recipient_node_id
request_id
responder_node_id
signature_algorithm
status
```

A result can complete the reference execution only when:

- the envelope is structurally valid;
- `request_id` matches the active execution;
- `responder_node_id` matches the attempted node;
- `recipient_node_id` matches the requesting node;
- `status == "OK"`;
- the responder public key is resolved by the trusted external key boundary;
- the Ed25519 signature verifies.

## 6. FailureNotice

Fields:

```text
protocol_version
message_id
request_id
reporting_node_id
failed_node_id
reason
observed_at
signature_algorithm
signature
```

Canonical key order:

```text
failed_node_id
message_id
observed_at
protocol_version
reason
reporting_node_id
request_id
signature_algorithm
```

Remote failure evidence is accepted only when:

- `request_id` matches the active execution;
- `failed_node_id` matches the attempted node;
- `reporting_node_id` is non-empty;
- the reporting-node public key is resolved by the trusted external key boundary;
- the Ed25519 signature verifies.

A valid signature does not override contract mismatches.

## 7. Local failure evidence

The initiating MA2A node may create a locally signed `FailureNotice` describing its own observation that an attempt could not be accepted.

Current reference reasons include:

- `endpoint_unavailable`;
- `transport_unavailable`;
- `invalid_result_contract`;
- `invalid_result_signature`;
- `invalid_failure_contract`;
- `invalid_failure_signature`;
- `unsupported_response_type`.

For local evidence:

- `reporting_node_id` is the initiating node;
- `failed_node_id` is the attempted target;
- the notice is signed by the initiating node.

This is an authenticated local observation. It is not represented as a signature made by the failed remote node.

## 8. Execution lifecycle

For each attempt:

```text
route
  -> begin attempt
  -> target request + re-sign
  -> transport
  -> authenticated result OR authenticated failure
```

On accepted failure, the attempted node is added to the execution-local exclusion set before asking routing for another target.

Routing MUST NOT return an already excluded target. MA2A treats that as a routing contract violation.

A successful authenticated result terminates the execution.

## 9. Terminal states

The v0.2 reference distinguishes:

- **completed**: authenticated successful result accepted;
- **route exhaustion**: routing returns no additional target before max attempts;
- **attempt exhaustion**: configured maximum number of attempts is reached.

Route exhaustion and attempt exhaustion are not the same condition.

## 10. Transport deadlines

The reference authenticated TCP executor uses configurable bounded deadlines.

Current defaults:

- connect timeout: 1500 ms;
- socket read/write timeout: 3000 ms.

These default values are implementation policy, not wire-format constants.

A silent peer must not be able to block the resilient execution indefinitely.

## 11. Security boundary

The C++ `PublicKeyResolver` is an injected trust boundary. v0.2 does not itself bind that resolver to the organizational PKI certificate chain.

The signed-message layer authenticates accepted evidence, but v0.2 does not claim transport confidentiality or channel binding.

See `security/SECURITY_REVIEW_v0.2.md`.

## 12. Compatibility

Python/C++ regression fixtures and signed TCP roundtrips guard the reference contract.

Changing field semantics, canonicalization, envelope structure, framing limits or accepted evidence rules requires a protocol-version decision rather than a silent implementation change.
