# MA2A Organizational PKI — v0.1 draft

## Trust hierarchy

```text
MA2A Official Root
        |
  Organization Certificate
        |
  Organization Memoria.ia / Intermediate
      /       |        \
   device    robot     agent
```

The MA2A Official Root identifies the official trust domain. An Organization Certificate binds an organization identifier, public key and policy metadata to that trust domain.

## Initial signature primitive

Ed25519 is the initial signing primitive for v0.1. Implementations must carry an algorithm identifier and must not hard-code protocol evolution to Ed25519 forever.

## Organization certificate fields

Minimum logical fields:

```text
certificate_version
serial
organization_id
subject_public_key
issuer_id
not_before
not_after
capabilities
policy_profile
status_endpoint_or_reference
signature_algorithm
issuer_signature
```

## Delegation

An organization may authorize subordinate device/agent credentials through an organizational intermediate key or equivalent delegated signing authority. Delegation depth and capability constraints must be policy-bounded.

## Network/commercial status

Cryptographic validity and admission status are distinct concepts. At minimum the official network may classify an organization as:

- `ACTIVE`
- `SUSPENDED`
- `EXPIRED`
- `REVOKED`

A certificate may be cryptographically well-formed yet denied access because its network/commercial status is not `ACTIVE`.

## Security boundaries

Private keys must never be committed to this repository. Root and organizational private-key storage require dedicated operational key-management procedures. The v0.1 repository contains formats, test keys only where unmistakably marked, and reference verification logic—not production secrets.

## Fork distinction

Possession of the public MA2A code/specification does not grant membership in the MA2A Official trust domain. Third parties may be technically able to create an independent root/network according to applicable license rights; such a root is not the MA2A Official Root.
