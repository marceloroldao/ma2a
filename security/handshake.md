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
6. delegated device/agent status is `ACTIVE`;
7. delegated credential has required capability;
8. transcript signature is valid;
9. client/server nonces match the current session;
10. protocol/capability negotiation is acceptable.

If all checks pass, the server returns `AUTH_OK` and binds the authenticated identity and authorization scope to the session. Otherwise it returns `AUTH_DENIED` without revealing unnecessary validation detail to an attacker.

## Replay protection

Challenges are single-use and short-lived. Servers must maintain sufficient replay state or an equivalent deterministic anti-replay mechanism.

## Revocation

Organization and delegated device/agent status are evaluated independently. A valid organization does not keep a revoked subordinate credential active, and a valid subordinate credential does not override a suspended or revoked organization.

A previously valid session may be terminated or denied renewal if either level becomes suspended/revoked according to the active network policy.

## Security note

This draft defines protocol intent and has an executable reference implementation, but it is not yet a production security review. v0.1 test coverage includes replay, altered transcript, wrong key, expired certificate, revoked organization, revoked device, unauthorized capability, malformed delegation and wrong trust root. Additional fuzzing, interoperability and operational key-management review remain required before production deployment.
