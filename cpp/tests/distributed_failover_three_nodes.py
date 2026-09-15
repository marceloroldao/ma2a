from __future__ import annotations

import base64
import json
import socket
import struct
import threading

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

MAX_FRAME = 1024 * 1024


def canonical(value: dict[str, object]) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def b64(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii")


def unb64(text: str) -> bytes:
    return base64.b64decode(text.encode("ascii"), altchars=b"-_", validate=True)


def recv_exact(sock: socket.socket, size: int) -> bytes:
    out = bytearray()
    while len(out) < size:
        chunk = sock.recv(size - len(out))
        if not chunk:
            raise RuntimeError("connection closed")
        out.extend(chunk)
    return bytes(out)


def recv_frame(sock: socket.socket) -> dict[str, object]:
    (length,) = struct.unpack("!I", recv_exact(sock, 4))
    if length <= 0 or length > MAX_FRAME:
        raise RuntimeError("invalid frame length")
    return json.loads(recv_exact(sock, length).decode("utf-8"))


def send_frame(sock: socket.socket, message: dict[str, object]) -> None:
    body = canonical(message)
    sock.sendall(struct.pack("!I", len(body)) + body)


def public_raw(key: Ed25519PrivateKey) -> bytes:
    return key.public_key().public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw,
    )


def sign_payload(payload: dict[str, object], key: Ed25519PrivateKey) -> dict[str, object]:
    signed = dict(payload)
    signed["signature"] = b64(key.sign(canonical(payload)))
    return signed


def verify_payload(payload: dict[str, object], public_key: bytes) -> None:
    unsigned = dict(payload)
    signature = unb64(str(unsigned.pop("signature")))
    Ed25519PrivateKey.from_private_bytes(bytes(32)).public_key()  # exercise backend initialization
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
    Ed25519PublicKey.from_public_bytes(public_key).verify(signature, canonical(unsigned))


def listen_once(handler):
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
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
    key_a = Ed25519PrivateKey.from_private_bytes(bytes(range(0, 32)))
    key_b = Ed25519PrivateKey.from_private_bytes(bytes(range(32, 64)))
    key_c = Ed25519PrivateKey.from_private_bytes(bytes(range(64, 96)))
    pub_a, pub_b, pub_c = public_raw(key_a), public_raw(key_b), public_raw(key_c)

    request_id = "job-distributed-failover-1"
    trace: list[str] = []

    # Node B accepts the signed request, verifies A, then reports a signed execution failure.
    def node_b(conn: socket.socket) -> None:
        message = recv_frame(conn)
        assert message["type"] == "JOB_REQUEST"
        request = dict(message["payload"])
        verify_payload(request, pub_a)
        assert request["target_node_id"] == "node-b"
        trace.append("B:request_verified")
        notice = {
            "failed_node_id": "node-b",
            "message_id": "failure-1",
            "observed_at": 1800000100,
            "protocol_version": "0.2",
            "reason": "execution_unavailable",
            "reporting_node_id": "node-b",
            "request_id": request_id,
            "signature_algorithm": "Ed25519",
        }
        send_frame(conn, {"type": "FAILURE_NOTICE", "payload": sign_payload(notice, key_b)})

    port_b, thread_b = listen_once(node_b)

    request_b = {
        "expires_at": 1800000200,
        "issued_at": 1800000000,
        "message_id": "request-b-1",
        "operation": "ECHO",
        "organization_id": "org-1",
        "payload": "failover-payload",
        "protocol_version": "0.2",
        "request_id": request_id,
        "sender_node_id": "node-a",
        "signature_algorithm": "Ed25519",
        "target_node_id": "node-b",
    }
    with socket.create_connection(("127.0.0.1", port_b), timeout=5) as conn:
        send_frame(conn, {"type": "JOB_REQUEST", "payload": sign_payload(request_b, key_a)})
        failure_envelope = recv_frame(conn)
    thread_b.join(timeout=5)

    assert failure_envelope["type"] == "FAILURE_NOTICE"
    failure = dict(failure_envelope["payload"])
    verify_payload(failure, pub_b)
    assert failure["request_id"] == request_id
    assert failure["failed_node_id"] == "node-b"
    trace.append("A:failure_verified")

    # Routing boundary: authenticated B is excluded; C is the next eligible node.
    candidates = [
        {"node_id": "node-b", "score": 100.0},
        {"node_id": "node-c", "score": 80.0},
    ]
    excluded = {str(failure["failed_node_id"])}
    eligible = [n for n in candidates if n["node_id"] not in excluded]
    selected = max(eligible, key=lambda n: (n["score"], n["node_id"]))["node_id"]
    assert selected == "node-c"
    trace.append("routing:B->C")

    # Node C verifies the rerouted request and returns a signed result to A.
    def node_c(conn: socket.socket) -> None:
        message = recv_frame(conn)
        assert message["type"] == "JOB_REQUEST"
        request = dict(message["payload"])
        verify_payload(request, pub_a)
        assert request["target_node_id"] == "node-c"
        trace.append("C:request_verified")
        result = {
            "completed_at": 1800000101,
            "message_id": "result-c-1",
            "payload": request["payload"],
            "protocol_version": "0.2",
            "recipient_node_id": "node-a",
            "request_id": request_id,
            "responder_node_id": "node-c",
            "signature_algorithm": "Ed25519",
            "status": "OK",
        }
        send_frame(conn, {"type": "JOB_RESULT", "payload": sign_payload(result, key_c)})

    port_c, thread_c = listen_once(node_c)
    request_c = dict(request_b)
    request_c["message_id"] = "request-c-1"
    request_c["target_node_id"] = "node-c"
    with socket.create_connection(("127.0.0.1", port_c), timeout=5) as conn:
        send_frame(conn, {"type": "JOB_REQUEST", "payload": sign_payload(request_c, key_a)})
        result_envelope = recv_frame(conn)
    thread_c.join(timeout=5)

    assert result_envelope["type"] == "JOB_RESULT"
    result = dict(result_envelope["payload"])
    verify_payload(result, pub_c)
    assert result["request_id"] == request_id
    assert result["responder_node_id"] == "node-c"
    assert result["status"] == "OK"
    assert result["payload"] == "failover-payload"
    trace.append("A:result_verified")

    assert trace == [
        "B:request_verified",
        "A:failure_verified",
        "routing:B->C",
        "C:request_verified",
        "A:result_verified",
    ]
    print("distributed signed failover OK:", " -> ".join(trace))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
