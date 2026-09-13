from __future__ import annotations

from dataclasses import asdict
import json

from ma2a.job import JobRequest


def test_python_job_request_envelope_matches_cpp_fixture() -> None:
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
    wire = json.dumps(
        {"type": "JOB_REQUEST", "payload": asdict(job)},
        sort_keys=True,
        separators=(",", ":"),
    )
    assert wire == (
        '{"payload":{"expires_at":1700000060,"issued_at":1700000000,'
        '"message_id":"msg-interop-1","operation":"ECHO",'
        '"organization_id":"org-1","payload":"hello",'
        '"protocol_version":"0.2","request_id":"job-interop-1",'
        '"sender_node_id":"node-python","signature":"abc123==",'
        '"signature_algorithm":"Ed25519","target_node_id":"node-cpp"},'
        '"type":"JOB_REQUEST"}'
    )
