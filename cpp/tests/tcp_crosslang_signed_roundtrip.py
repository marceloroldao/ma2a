from __future__ import annotations

import base64
import json
import socket
import struct
import subprocess
import sys

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey

MAX_FRAME = 1024 * 1024
CPP_PUBLIC_KEY = bytes.fromhex("03a107bff3ce10be1d70dd18e74bc09967e4d6309ba50d5f1ddc8664125531b8")
PYTHON_SEED = bytes(range(32, 64))


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


def recv_frame(sock: socket.socket) -> bytes:
    (length,) = struct.unpack("!I", recv_exact(sock, 4))
    if length <= 0 or length > MAX_FRAME:
        raise RuntimeError("invalid frame length")
    return recv_exact(sock, length)


def send_frame(sock: socket.socket, body: bytes) -> None:
    if not body or len(body) > MAX_FRAME:
        raise RuntimeError("invalid outbound frame")
    sock.sendall(struct.pack("!I", len(body)) + body)


def main() -> int:
    if len(sys.argv) != 2:
        return 2
    client_exe = sys.argv[1]

    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]

    proc = subprocess.Popen([client_exe, str(port)])
    try:
        conn, _ = listener.accept()
        with conn:
            message = json.loads(recv_frame(conn).decode("utf-8"))
            assert set(message) == {"payload", "type"}
            assert message["type"] == "JOB_REQUEST"
            request = message["payload"]
            signature = request.pop("signature")
            Ed25519PublicKey.from_public_bytes(CPP_PUBLIC_KEY).verify(
                unb64(signature), canonical(request)
            )
            assert request["sender_node_id"] == "node-cpp"
            assert request["target_node_id"] == "node-python"
            assert request["operation"] == "ECHO"
            assert request["payload"] == "signed-hello"

            result = {
                "completed_at": 1800000001,
                "message_id": "msg-signed-result-1",
                "payload": request["payload"],
                "protocol_version": "0.2",
                "recipient_node_id": "node-cpp",
                "request_id": request["request_id"],
                "responder_node_id": "node-python",
                "signature_algorithm": "Ed25519",
                "status": "OK",
            }
            private_key = Ed25519PrivateKey.from_private_bytes(PYTHON_SEED)
            result["signature"] = b64(private_key.sign(canonical(result)))
            envelope = {"payload": result, "type": "JOB_RESULT"}
            send_frame(conn, canonical(envelope))
    finally:
        listener.close()

    return proc.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
