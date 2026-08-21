# Security Policy

MA2A is experimental protocol research and is not yet security-certified for production use.

## Scope

Security-sensitive areas include:

- organizational PKI and certificate validation;
- Ed25519 signing and verification;
- challenge-response authentication;
- replay protection;
- privacy-scope enforcement;
- message canonicalization;
- delegated device/agent credentials;
- revocation and suspension;
- tenant/organization isolation.

## Secret handling

Production private keys, seed phrases, credentials, API tokens, root keys, organization signing keys, or real customer certificates MUST NOT be committed to this repository.

Test keys, if introduced, must be clearly marked as non-production fixtures and must never be reused outside tests.

## Current status

The v0.1 development line requires negative tests for at least:

- replayed challenge;
- reused message ID;
- altered signed transcript;
- wrong signing key;
- expired certificate;
- revoked/suspended organization;
- unauthorized capability;
- malformed certificate chain;
- scope escalation;
- ambiguous serialization/canonicalization.

Production deployment requires an independent security review and deployment-specific key-management design.
