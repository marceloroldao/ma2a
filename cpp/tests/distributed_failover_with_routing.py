from __future__ import annotations

import base64
import json
import socket
import struct
import subprocess
import sys
import threading

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey

MAX_FRAME = 1024 * 1024


def canonical(value: dict[str, object]) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


def b64(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode()


def unb64(text: str) -> bytes:
    return base64.b64decode(text.encode(), altchars=b"-_", validate=True)


def public_raw(key: Ed25519PrivateKey) -> bytes:
    return key.public_key().public_bytes(serialization.Encoding.Raw, serialization.PublicFormat.Raw)


def sign(payload: dict[str, object], key: Ed25519PrivateKey) -> dict[str, object]:
    out = dict(payload)
    out["signature"] = b64(key.sign(canonical(payload)))
    return out


def verify(payload: dict[str, object], public_key: bytes) -> None:
    unsigned = dict(payload)
    signature = unb64(str(unsigned.pop("signature")))
    Ed25519PublicKey.from_public_bytes(public_key).verify(signature, canonical(unsigned))


def recv_exact(sock: socket.socket, size: int) -> bytes:
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise RuntimeError("connection closed")
        data.extend(chunk)
    return bytes(data)


def recv_frame(sock: socket.socket) -> dict[str, object]:
    (length,) = struct.unpack("!I", recv_exact(sock, 4))
    if length <= 0 or length > MAX_FRAME:
        raise RuntimeError("invalid frame")
    return json.loads(recv_exact(sock, length))


def send_frame(sock: socket.socket, message: dict[str, object]) -> None:
    body = canonical(message)
    sock.sendall(struct.pack("!I", len(body)) + body)


def listen_once(handler):
    listener = socket.socket()
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]

    def run():
        try:
            conn, _ = listener.accept()
            with conn:
                handler(conn)
        finally:
            listener.close()

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    return port, thread


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: distributed_failover_with_routing.py <routing-driver>")
    routing_driver = sys.argv[1]

    key_a = Ed25519PrivateKey.from_private_bytes(bytes(range(32)))
    key_b = Ed25519PrivateKey.from_private_bytes(bytes(range(32, 64)))
    key_c = Ed25519PrivateKey.from_private_bytes(bytes(range(64, 96)))
    pub_a, pub_b, pub_c = public_raw(key_a), public_raw(key_b), public_raw(key_c)
    request_id = "job-real-routing-1"

    def node_b(conn: socket.socket) -> None:
        envelope = recv_frame(conn)
        request = dict(envelope["payload"])
        verify(request, pub_a)
        assert request["target_node_id"] == "node-b"
        notice = {
            "failed_node_id": "node-b", "message_id": "failure-real-1",
            "observed_at": 1800000100, "protocol_version": "0.2",
            "reason": "execution_unavailable", "reporting_node_id": "node-b",
            "request_id": request_id, "signature_algorithm": "Ed25519",
        }
        send_frame(conn, {"type": "FAILURE_NOTICE", "payload": sign(notice, key_b)})

    request = {
        "expires_at": 1800000200, "issued_at": 1800000000, "message_id": "request-real-b",
        "operation": "ECHO", "organization_id": "org-1", "payload": "real-routing-payload",
        "protocol_version": "0.2", "request_id": request_id, "sender_node_id": "node-a",
        "signature_algorithm": "Ed25519", "target_node_id": "node-b",
    }

    port_b, tb = listen_once(node_b)
    with socket.create_connection(("127.0.0.1", port_b), timeout=5) as conn:
        send_frame(conn, {"type": "JOB_REQUEST", "payload": sign(request, key_a)})
        failure_envelope = recv_frame(conn)
    tb.join(5)
    failure = dict(failure_envelope["payload"])
    verify(failure, pub_b)

    routed = subprocess.run(
        [routing_driver, request_id, str(failure["failed_node_id"]), str(failure["reason"]), str(failure["observed_at"])],
        check=True, capture_output=True, text=True,
    ).stdout.strip().splitlines()
    assert routed == ["node-b", "node-c"], routed
    selected = routed[1]

    def node_c(conn: socket.socket) -> None:
        envelope = recv_frame(conn)
        rerouted = dict(envelope["payload"])
        verify(rerouted, pub_a)
        assert rerouted["target_node_id"] == selected
        result = {
            "completed_at": 1800000101, "message_id": "result-real-c",
            "payload": rerouted["payload"], "protocol_version": "0.2",
            "recipient_node_id": "node-a", "request_id": request_id,
            "responder_node_id": selected, "signature_algorithm": "Ed25519", "status": "OK",
        }
        send_frame(conn, {"type": "JOB_RESULT", "payload": sign(result, key_c)})

    port_c, tc = listen_once(node_c)
    rerouted_request = dict(request)
    rerouted_request["message_id"] = "request-real-c"
    rerouted_request["target_node_id"] = selected
    with socket.create_connection(("127.0.0.1", port_c), timeout=5) as conn:
        send_frame(conn, {"type": "JOB_REQUEST", "payload": sign(rerouted_request, key_a)})
        result_envelope = recv_frame(conn)
    tc.join(5)

    result = dict(result_envelope["payload"])
    verify(result, pub_c)
    assert result["status"] == "OK"
    assert result["responder_node_id"] == "node-c"
    assert result["request_id"] == request_id
    assert result["payload"] == "real-routing-payload"
    print("MA2A + resolutive-routing real failover OK: A -> B -> failure -> routing -> C -> A")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
