import subprocess
import sys

from ma2a.wire import ReferenceDelta, decode_reference_delta, encode_reference_delta


def test_reference_delta_roundtrips_across_independent_processes():
    sender_code = r'''
from ma2a.wire import ReferenceDelta, encode_reference_delta

delta = ReferenceDelta(
    protocol_version="0.1",
    message_id="msg-001",
    sender_id="device:robot-7",
    organization_id="org:acme",
    trajectory_address="traj:42",
    base_version=7,
    new_version=8,
    operation="PATCH",
    scope="shared",
    payload_or_reference="ref:payload-8",
    integrity_digest="sha256:example",
    logical_clock=91,
    signature="ed25519:example",
)
import sys
sys.stdout.buffer.write(encode_reference_delta(delta))
'''
    wire = subprocess.check_output([sys.executable, "-c", sender_code])

    receiver_code = r'''
from ma2a.wire import decode_reference_delta, encode_reference_delta
import sys
wire = sys.stdin.buffer.read()
delta = decode_reference_delta(wire)
sys.stdout.buffer.write(encode_reference_delta(delta))
'''
    echoed = subprocess.check_output(
        [sys.executable, "-c", receiver_code],
        input=wire,
    )

    assert echoed == wire
    decoded = decode_reference_delta(echoed)
    assert decoded.organization_id == "org:acme"
    assert decoded.trajectory_address == "traj:42"
    assert decoded.base_version == 7
    assert decoded.new_version == 8


def test_reference_encoding_is_deterministic():
    delta = ReferenceDelta(
        protocol_version="0.1",
        message_id="msg-002",
        sender_id="agent:a",
        organization_id="org:a",
        trajectory_address="traj:x",
        base_version=0,
        new_version=1,
        operation="SET",
        scope="shared",
        payload_or_reference="payload",
        integrity_digest="digest",
        logical_clock=1,
        signature="sig",
    )
    first = encode_reference_delta(delta)
    second = encode_reference_delta(decode_reference_delta(first))
    assert first == second
