from __future__ import annotations

from dataclasses import asdict, dataclass
import json


_ALLOWED_OPERATIONS = {"SET", "PATCH", "DELETE", "LINK", "TOMBSTONE"}
_ALLOWED_SCOPES = {"private", "user", "shared", "global"}
MAX_REFERENCE_DELTA_BYTES = 64 * 1024


@dataclass(frozen=True)
class ReferenceDelta:
    protocol_version: str
    message_id: str
    sender_id: str
    organization_id: str
    trajectory_address: str
    base_version: int
    new_version: int
    operation: str
    scope: str
    payload_or_reference: str
    integrity_digest: str
    logical_clock: int
    signature: str

    def validate(self) -> None:
        if self.operation not in _ALLOWED_OPERATIONS:
            raise ValueError("invalid operation")
        if self.scope not in _ALLOWED_SCOPES:
            raise ValueError("invalid scope")
        if self.base_version < 0 or self.new_version < 0:
            raise ValueError("negative version")
        if self.logical_clock < 0:
            raise ValueError("negative logical clock")
        if not self.message_id or not self.sender_id or not self.organization_id:
            raise ValueError("missing identity field")
        if not self.trajectory_address:
            raise ValueError("missing trajectory address")


def encode_reference_delta(delta: ReferenceDelta) -> bytes:
    """Encode a test/reference MA2A delta as deterministic JSON.

    This is deliberately non-normative. It exists to exercise independent
    process interoperability before a compact v0.1 wire encoding is frozen.
    """
    delta.validate()
    encoded = json.dumps(
        asdict(delta),
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
    ).encode("utf-8")
    if len(encoded) > MAX_REFERENCE_DELTA_BYTES:
        raise ValueError("reference delta too large")
    return encoded


def decode_reference_delta(data: bytes) -> ReferenceDelta:
    if not isinstance(data, (bytes, bytearray)):
        raise ValueError("reference delta must be bytes")
    if len(data) > MAX_REFERENCE_DELTA_BYTES:
        raise ValueError("reference delta too large")
    try:
        raw = json.loads(bytes(data).decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError, TypeError) as exc:
        raise ValueError("malformed reference delta") from exc
    if not isinstance(raw, dict):
        raise ValueError("reference delta must be an object")
    required = {
        "protocol_version",
        "message_id",
        "sender_id",
        "organization_id",
        "trajectory_address",
        "base_version",
        "new_version",
        "operation",
        "scope",
        "payload_or_reference",
        "integrity_digest",
        "logical_clock",
        "signature",
    }
    if set(raw) != required:
        raise ValueError("unexpected or missing fields")
    try:
        delta = ReferenceDelta(**raw)
        delta.validate()
    except (TypeError, AttributeError) as exc:
        raise ValueError("invalid reference delta fields") from exc
    return delta
