# Signed cross-language TCP roundtrip

The native interoperability test exercises the following protocol path over loopback TCP:

1. C++ builds a canonical `JobRequest`.
2. C++ signs the unsigned canonical payload with Ed25519.
3. Python receives the framed envelope and verifies the C++ signature.
4. Python creates a canonical `JobResult` and signs it with a distinct Ed25519 key.
5. C++ receives the framed result and verifies the Python signature.

This test validates framing, canonical serialization, and Ed25519 interoperability in one end-to-end path. It intentionally uses only the reference-safe `ECHO` operation.
