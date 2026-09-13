from __future__ import annotations

from dataclasses import asdict
import json
import socket
import struct
from typing import Callable

from .job import JobRequest, JobResult

MAX_FRAME_BYTES = 1024 * 1024


def _recv_exact(sock: socket.socket, size: int) -> bytes:
    chunks: list[bytes] = []
    remaining = size
    while remaining:
        chunk = sock.recv(remaining)
        if not chunk:
            raise ConnectionError("connection closed before frame completed")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def send_frame(sock: socket.socket, message_type: str, payload: dict[str, object]) -> None:
    body = json.dumps(
        {"type": message_type, "payload": payload},
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    if len(body) > MAX_FRAME_BYTES:
        raise ValueError("frame too large")
    sock.sendall(struct.pack("!I", len(body)) + body)


def recv_frame(sock: socket.socket) -> tuple[str, dict[str, object]]:
    (length,) = struct.unpack("!I", _recv_exact(sock, 4))
    if length <= 0 or length > MAX_FRAME_BYTES:
        raise ValueError("invalid frame length")
    raw = json.loads(_recv_exact(sock, length).decode("utf-8"))
    if not isinstance(raw, dict) or set(raw) != {"type", "payload"}:
        raise ValueError("invalid frame envelope")
    if not isinstance(raw["type"], str) or not isinstance(raw["payload"], dict):
        raise ValueError("invalid frame fields")
    return raw["type"], raw["payload"]


def send_job(host: str, port: int, request: JobRequest, *, timeout: float = 2.0) -> JobResult:
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.settimeout(timeout)
        send_frame(sock, "JOB_REQUEST", asdict(request))
        message_type, payload = recv_frame(sock)
        if message_type != "JOB_RESULT":
            raise ValueError("unexpected response type")
        return JobResult(**payload)


def serve_one_job(
    listener: socket.socket,
    handler: Callable[[JobRequest], JobResult],
    *,
    timeout: float = 2.0,
) -> None:
    listener.settimeout(timeout)
    conn, _ = listener.accept()
    with conn:
        conn.settimeout(timeout)
        message_type, payload = recv_frame(conn)
        if message_type != "JOB_REQUEST":
            raise ValueError("unexpected request type")
        request = JobRequest(**payload)
        result = handler(request)
        send_frame(conn, "JOB_RESULT", asdict(result))


def open_listener(host: str = "127.0.0.1", port: int = 0, *, backlog: int = 4) -> socket.socket:
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((host, port))
    listener.listen(backlog)
    return listener
