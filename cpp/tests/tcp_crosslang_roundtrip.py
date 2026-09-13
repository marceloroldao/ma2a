from __future__ import annotations

import json
import socket
import struct
import subprocess
import sys


def recv_exact(sock: socket.socket, size: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < size:
        chunk = sock.recv(size - len(chunks))
        if not chunk:
            raise RuntimeError("connection closed")
        chunks.extend(chunk)
    return bytes(chunks)


def recv_frame(sock: socket.socket) -> bytes:
    (length,) = struct.unpack("!I", recv_exact(sock, 4))
    if length <= 0 or length > 1024 * 1024:
        raise RuntimeError("invalid frame length")
    return recv_exact(sock, length)


def send_frame(sock: socket.socket, payload: bytes) -> None:
    sock.sendall(struct.pack("!I", len(payload)) + payload)


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
            body = recv_frame(conn)
            message = json.loads(body.decode("utf-8"))
            assert message["type"] == "JOB_REQUEST"
            payload = message["payload"]
            assert payload["sender_node_id"] == "node-cpp"
            assert payload["target_node_id"] == "node-python"
            assert payload["operation"] == "ECHO"
            assert payload["payload"] == "hello-over-tcp"

            result = {
                "payload": {
                    "completed_at": 1700000001,
                    "message_id": "msg-result-1",
                    "payload": "hello-over-tcp",
                    "protocol_version": "0.2",
                    "recipient_node_id": "node-cpp",
                    "request_id": "job-tcp-1",
                    "responder_node_id": "node-python",
                    "signature": "fixture-result-signature",
                    "signature_algorithm": "Ed25519",
                    "status": "OK",
                },
                "type": "JOB_RESULT",
            }
            encoded = json.dumps(result, sort_keys=True, separators=(",", ":")).encode("utf-8")
            send_frame(conn, encoded)
    finally:
        listener.close()

    return proc.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
