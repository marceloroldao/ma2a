from __future__ import annotations

import socket
import struct

import pytest

from ma2a.transport import MAX_FRAME_BYTES, recv_frame, send_frame


def test_reference_frame_limit_is_one_mebibyte() -> None:
    assert MAX_FRAME_BYTES == 1024 * 1024


def test_reference_rejects_zero_length_frame() -> None:
    left, right = socket.socketpair()
    try:
        left.sendall(struct.pack("!I", 0))
        with pytest.raises(ValueError, match="invalid frame length"):
            recv_frame(right)
    finally:
        left.close()
        right.close()


def test_reference_rejects_oversized_outbound_frame() -> None:
    left, right = socket.socketpair()
    try:
        huge = "x" * (MAX_FRAME_BYTES + 1)
        with pytest.raises(ValueError, match="frame too large"):
            send_frame(left, "TEST", {"payload": huge})
    finally:
        left.close()
        right.close()
