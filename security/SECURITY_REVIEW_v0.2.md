# MA2A Security Review — v0.2 Release Candidate

Status: INTERNAL ENGINEERING REVIEW / NOT AN EXTERNAL AUDIT

## Scope

This review covers the v0.2 resilient-execution boundary:

- signed `JobRequest`;
- signed `JobResult`;
- signed `FailureNotice`;
- framed TCP attempts;
- route/failover orchestration;
- public-key resolution boundary;
- malformed/forged response handling.

It does not certify the whole MA2A architecture for production deployment.

## Controls present

### Message authenticity

Execution messages use Ed25519 signatures over canonical JSON.

A forged result or failure notice is rejected when the resolved public key does not verify the signature.

### Contract binding

A valid signature alone is insufficient.

The executor also checks:

- active `request_id`;
- attempted/responder node relationship;
- result recipient;
- successful result status;
- failed-node identity for failure evidence.

### Target-bound re-signing

The request is re-signed after every routing decision because `target_node_id` is part of the signed payload.

This prevents a correctly signed request for one node from being silently reused as a request for another node.

### Frame bounds

The reference wire rejects zero-length frames and bodies above 1 MiB before reading the announced body.

### Bounded blocking

The authenticated TCP executor uses bounded connect and socket I/O deadlines.

A peer that accepts a connection and remains silent cannot block the execution indefinitely under the configured policy.

### SIGPIPE handling

Failed Linux TCP sends use a no-SIGPIPE path so a broken peer does not terminate the MA2A process.

### Adversarial regression gate

The release line tests:

- missing endpoint;
- connection refusal;
- malformed response envelope;
- oversized declared frame;
- unsupported response type;
- signed result with wrong request ID;
- forged result signature;
- valid remote failure evidence;
- forged failure signature;
- validly signed failure for the wrong attempted node;
- valid authenticated result;
- route exhaustion;
- max-attempt exhaustion.

## Trust assumptions

### PublicKeyResolver

The C++ executor receives a `PublicKeyResolver` from the embedding system.

The resolver is trusted to map an authenticated node identity to the correct Ed25519 public key.

The v0.2 executor does not itself validate an organization/device certificate chain before calling the resolver. Production integration must bind this resolver to the MA2A PKI/admission state.

### EndpointResolver

The endpoint resolver is also supplied externally.

A malicious or incorrect resolver can direct traffic to the wrong address. Signed response verification limits evidence forgery, but it does not provide confidentiality or availability.

### Local failure evidence

Transport/protocol rejection is represented by a `FailureNotice` signed by the initiating node.

This records a local authenticated observation; it is not cryptographic proof that the remote node itself failed.

## Known gaps

### No transport confidentiality

The current reference uses TCP framing, not an encrypted authenticated channel.

Ed25519 message signatures protect authenticity of accepted evidence, but passive observers may see payload contents and active attackers may drop/delay traffic.

A production profile needs TLS/QUIC or another channel-security design plus explicit identity/channel binding.

### No production C++ execution listener

The release candidate demonstrates the client/executor boundary and interoperable test peers.

It does not yet ship a production C++ server/listener that combines:

- certificate admission;
- request signature verification;
- expiry validation;
- replay cache;
- authorization;
- execution sandbox/policy.

### Replay protection is not frozen for execution transport

The wider MA2A reference already has replay concepts in the admission/synchronization layer, but the v0.2 C++ execution transport does not yet define a persistent distributed replay store for `message_id`/`request_id`.

### Key custody

Production root/organization/device private-key storage, rotation and HSM/KMS integration remain outside the reference implementation.

### Revocation distribution

The release candidate does not define a production distributed revocation/status propagation service.

### Resource exhaustion beyond frame size

The 1 MiB frame bound limits single-frame allocation, but production listeners still require:

- connection limits;
- per-peer rate limits;
- queue limits;
- execution quotas;
- backpressure.

### External audit

No independent cryptographic or protocol security audit has been completed.

## RC conclusion

The v0.2 candidate is suitable as a reproducible experimental baseline for authenticated resilient-execution research and integration testing.

It must not be described as production-grade secure networking.
