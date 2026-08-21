# MA2A Authentication Handshake — v0.1 draft

## Message flow

```text
CLIENT -> HELLO
SERVER -> AUTH_CHALLENGE
CLIENT -> AUTH_RESPONSE
SERVER -> AUTH_OK | AUTH_DENIED
```

## HELLO

A client presents, at minimum:

```text
protocol_version
organization_id
agent_or_device_id
certificate_chain
client_nonce
capabilities
```

No private key is transmitted.

## AUTH_CHALLENGE

The server returns a fresh challenge bound to the attempted session:

```text
server_nonce
challenge_id
issued_at
expires_at
selected_protocol_version
selected_capabilities
```

## AUTH_RESPONSE

The client signs a canonical transcript including both nonces and the selected session parameters:

```text
T = canonical(
  protocol_version,
  organization_id,
  agent_or_device_id,
  client_nonce,
  server_nonce,
  challenge_id,
  selected_capabilities
)

signature = Sign(private_key, T)
```

For v0.1 the initial signature primitive is Ed25519.

## Server verification

The server must verify:

1. challenge exists and is unexpired;
2. challenge has not already been consumed;
3. certificate chain terminates in an accepted MA2A Official trust anchor;
4. certificate validity interval and constraints;
5. organization/network status is `ACTIVE`;
6. delegated credential has required capability;
7. transcript signature is valid;
8. client/server nonces match the current session;
9. protocol/capability negotiation is acceptable.

If all checks pass, the server returns `AUTH_OK` and binds the authenticated identity and authorization scope to the session. Otherwise it returns `AUTH_DENIED` without revealing unnecessary validation detail to an attacker.

## Replay protection

Challenges are single-use and short-lived. Servers must maintain sufficient replay state or an equivalent deterministic anti-replay mechanism.

## Revocation

A previously valid session may be terminated or denied renewal if the organization or delegated credential becomes suspended/revoked according to the active network policy.

## Security note

This draft defines protocol intent. It is not yet a security-reviewed production authentication protocol. v0.1 must include negative tests for replay, altered transcript, wrong key, expired certificate, revoked organization, unauthorized capability and malformed chain.
