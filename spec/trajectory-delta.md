# MA2A Trajectory Delta — v0.1 draft

A trajectory delta is the minimal versioned mutation unit exchanged by MA2A synchronization peers.

## Required fields

```text
protocol_version : uint/string
message_id       : unique identifier
sender_id        : cryptographic identity
organization_id  : organization identity
trajectory_address : resolutive address
base_version     : prior accepted version
new_version      : proposed version
operation        : SET | PATCH | DELETE | LINK | TOMBSTONE
scope            : private | user | shared | global
payload_or_reference : bytes/reference
integrity_digest : digest of canonical content
logical_clock    : deterministic ordering metadata
signature        : signature over canonical envelope
```

## Canonicalization

A signature MUST cover a deterministic canonical representation of all security-relevant fields. Implementations MUST NOT sign an ambiguous serialization.

The final binary encoding is not frozen in v0.1. Candidate encodings may include CBOR, MessagePack, Protobuf or a custom compact format after benchmark and canonicalization review.

## Validation order

1. parse with strict bounds;
2. validate protocol version;
3. authenticate identity/certificate chain;
4. verify signature;
5. verify replay/message ID constraints;
6. verify integrity digest;
7. enforce scope and authorization;
8. resolve trajectory address;
9. compare `base_version` with local accepted version;
10. apply deterministic conflict rule;
11. persist/relay only if accepted.

## Privacy rule

`private` deltas MUST NOT be forwarded to L3 by default. Scope broadening requires an explicit authorized action.
