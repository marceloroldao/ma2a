from __future__ import annotations

from dataclasses import asdict
import json

from ma2a.job import JobResult


def test_python_job_result_envelope_matches_cpp_fixture() -> None:
    result = JobResult(
        protocol_version="0.2",
        message_id="result-1",
        request_id="job-interop-1",
        responder_node_id="node-cpp",
        recipient_node_id="node-python",
        status="OK",
        payload="hello",
        completed_at=1700000001,
        signature_algorithm="Ed25519",
        signature="xyz789==",
    )
    wire = json.dumps(
        {"type": "JOB_RESULT", "payload": asdict(result)},
        sort_keys=True,
        separators=(",", ":"),
    )
    assert wire == (
        '{"payload":{"completed_at":1700000001,"message_id":"result-1",'
        '"payload":"hello","protocol_version":"0.2",'
        '"recipient_node_id":"node-python","request_id":"job-interop-1",'
        '"responder_node_id":"node-cpp","signature":"xyz789==",'
        '"signature_algorithm":"Ed25519","status":"OK"},'
        '"type":"JOB_RESULT"}'
    )
