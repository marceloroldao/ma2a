from __future__ import annotations

import json

from ma2a.transport import MAX_FRAME_BYTES


def test_frame_budget_is_defined_on_encoded_body_bytes() -> None:
    body = json.dumps({"type": "X", "payload": {"x": "a"}}, sort_keys=True, separators=(",", ":")).encode("utf-8")
    assert len(body) < MAX_FRAME_BYTES
