import json
import random

import pytest

from ma2a.wire import MAX_REFERENCE_DELTA_BYTES, ReferenceDelta, decode_reference_delta, encode_reference_delta


def valid_delta():
    return ReferenceDelta(
        protocol_version="0.1",
        message_id="m",
        sender_id="device:d",
        organization_id="org:o",
        trajectory_address="traj:t",
        base_version=0,
        new_version=1,
        operation="SET",
        scope="shared",
        payload_or_reference="payload",
        integrity_digest="digest",
        logical_clock=1,
        signature="sig",
    )


def test_oversized_reference_delta_is_rejected():
    with pytest.raises(ValueError):
        decode_reference_delta(b"{" + b"x" * MAX_REFERENCE_DELTA_BYTES + b"}")


def test_missing_and_extra_fields_are_rejected():
    raw = json.loads(encode_reference_delta(valid_delta()).decode())
    raw.pop("signature")
    with pytest.raises(ValueError):
        decode_reference_delta(json.dumps(raw).encode())

    raw = json.loads(encode_reference_delta(valid_delta()).decode())
    raw["unexpected"] = 1
    with pytest.raises(ValueError):
        decode_reference_delta(json.dumps(raw).encode())


def test_invalid_operation_scope_and_versions_are_rejected():
    with pytest.raises(ValueError):
        encode_reference_delta(valid_delta().__class__(**{**valid_delta().__dict__, "operation": "EXECUTE"}))
    with pytest.raises(ValueError):
        encode_reference_delta(valid_delta().__class__(**{**valid_delta().__dict__, "scope": "secret"}))
    with pytest.raises(ValueError):
        encode_reference_delta(valid_delta().__class__(**{**valid_delta().__dict__, "new_version": -1}))


def test_seeded_byte_mutations_never_escape_as_unexpected_exceptions():
    original = bytearray(encode_reference_delta(valid_delta()))
    rng = random.Random(20260821)
    for _ in range(100):
        candidate = bytearray(original)
        for _ in range(rng.randint(1, 4)):
            idx = rng.randrange(len(candidate))
            candidate[idx] = rng.randrange(256)
        try:
            decoded = decode_reference_delta(bytes(candidate))
        except ValueError:
            continue
        # If a mutation remains syntactically and semantically valid, it must
        # still round-trip through the canonical reference encoder.
        assert decode_reference_delta(encode_reference_delta(decoded)) == decoded
