from __future__ import annotations

import threading
import time
import unittest

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from ma2a.job import (
    JobRequest,
    JobResult,
    execute_reference_job,
    sign_job_request,
    sign_job_result,
    verify_job_request,
    verify_job_result,
)
from ma2a.transport import open_listener, send_job, serve_one_job


class TwoNodeJobTransportTests(unittest.TestCase):
    def test_signed_echo_roundtrip_between_two_nodes(self) -> None:
        key_a = Ed25519PrivateKey.generate()
        key_b = Ed25519PrivateKey.generate()
        now = int(time.time())
        request = sign_job_request(
            JobRequest(
                protocol_version="0.1",
                message_id="msg-1",
                request_id="req-1",
                sender_node_id="node-a",
                target_node_id="node-b",
                organization_id="org-1",
                operation="ECHO",
                payload="hello-m2a2",
                issued_at=now,
                expires_at=now + 30,
            ),
            key_a,
        )

        listener = open_listener()
        port = listener.getsockname()[1]
        errors: list[BaseException] = []

        def handler(incoming: JobRequest) -> JobResult:
            self.assertEqual(incoming.target_node_id, "node-b")
            self.assertTrue(verify_job_request(incoming, key_a.public_key(), now=now))
            status, payload = execute_reference_job(incoming)
            return sign_job_result(
                JobResult(
                    protocol_version="0.1",
                    message_id="msg-2",
                    request_id=incoming.request_id,
                    responder_node_id="node-b",
                    recipient_node_id=incoming.sender_node_id,
                    status=status,
                    payload=payload,
                    completed_at=now + 1,
                ),
                key_b,
            )

        def server() -> None:
            try:
                serve_one_job(listener, handler)
            except BaseException as exc:  # pragma: no cover - surfaced below
                errors.append(exc)
            finally:
                listener.close()

        thread = threading.Thread(target=server, daemon=True)
        thread.start()
        result = send_job("127.0.0.1", port, request)
        thread.join(timeout=3)

        if errors:
            raise errors[0]
        self.assertFalse(thread.is_alive())
        self.assertEqual(result.status, "OK")
        self.assertEqual(result.payload, "hello-m2a2")
        self.assertEqual(result.request_id, request.request_id)
        self.assertTrue(verify_job_result(result, key_b.public_key()))

    def test_tampered_job_request_fails_signature(self) -> None:
        key = Ed25519PrivateKey.generate()
        now = int(time.time())
        signed = sign_job_request(
            JobRequest("0.1", "m", "r", "a", "b", "org", "PING", "", now, now + 30),
            key,
        )
        tampered = JobRequest(**{**signed.__dict__, "target_node_id": "node-c"})
        self.assertFalse(verify_job_request(tampered, key.public_key(), now=now))

    def test_expired_job_request_is_rejected(self) -> None:
        key = Ed25519PrivateKey.generate()
        signed = sign_job_request(
            JobRequest("0.1", "m", "r", "a", "b", "org", "PING", "", 10, 20),
            key,
        )
        self.assertFalse(verify_job_request(signed, key.public_key(), now=21))

    def test_arbitrary_operation_is_not_allowed(self) -> None:
        key = Ed25519PrivateKey.generate()
        with self.assertRaises(ValueError):
            sign_job_request(
                JobRequest("0.1", "m", "r", "a", "b", "org", "EXEC", "rm -rf /", 10, 20),
                key,
            )


if __name__ == "__main__":
    unittest.main()
