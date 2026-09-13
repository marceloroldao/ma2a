# MA2A protocol limits

These limits are normative for the reference wire contract and must remain identical across language implementations.

- Frame length prefix: 4-byte unsigned big-endian integer.
- Maximum frame body: 1 MiB (`1024 * 1024` bytes).
- Zero-length frames are invalid.
- Reference job operations: `PING`, `ECHO` only.
- Signature algorithm: Ed25519.
- Signed structured payloads use canonical UTF-8 JSON with keys sorted lexicographically and separators `,` and `:` without extra whitespace.

Any implementation accepting a wider wire envelope is not protocol-compatible even if it can decode valid reference messages.
