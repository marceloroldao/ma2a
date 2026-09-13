from dataclasses import asdict
import socket
import struct

from ma2a.job import JobRequest
from ma2a.transport import MAX_FRAME_BYTES, send_frame


def _recv_exact(sock: socket.socket, size: int) -> bytes:
    out = bytearray()
    while len(out) < size:
        out.extend(sock.recv(size - len(out)))
    return bytes(out)


def test_job_request_wire_contract_is_canonical_and_matches_native_fixture() -> None:
    job = JobRequest(
        protocol_version="0.2",
        message_id="msg-interop-1",
        request_id="job-interop-1",
        sender_node_id="node-python",
        target_node_id="node-cpp",
        organization_id="org-1",
        operation="ECHO",
        payload="hello",
        issued_at=1700000000,
        expires_at=1700000060,
        signature_algorithm="Ed25519",
        signature="abc123==",
    )
    expected = (
        b'{"payload":{"expires_at":1700000060,"issued_at":1700000000,'
        b'"message_id":"msg-interop-1","operation":"ECHO","organization_id":"org-1",'
        b'"payload":"hello","protocol_version":"0.2","request_id":"job-interop-1",'
        b'"sender_node_id":"node-python","signature":"abc123==",'
        b'"signature_algorithm":"Ed25519","target_node_id":"node-cpp"},"type":"JOB_REQUEST"}'
    )
    left, right = socket.socketpair()
    try:
        send_frame(left, "JOB_REQUEST", asdict(job))
        (length,) = struct.unpack("!I", _recv_exact(right, 4))
        body = _recv_exact(right, length)
    finally:
        left.close()
        right.close()
    assert body == expected
    assert MAX_FRAME_BYTES == 1024 * 1024
