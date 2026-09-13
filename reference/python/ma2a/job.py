from __future__ import annotations

from dataclasses import asdict, dataclass
import base64
import binascii

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey

from .auth import canonical_json


def _b64(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii")


def _unb64(text: str) -> bytes:
    return base64.b64decode(text.encode("ascii"), altchars=b"-_", validate=True)


_ALLOWED_OPERATIONS = {"PING", "ECHO"}
_ALLOWED_STATUS = {"OK", "ERROR"}


@dataclass(frozen=True)
class JobRequest:
    protocol_version: str
    message_id: str
    request_id: str
    sender_node_id: str
    target_node_id: str
    organization_id: str | None
    operation: str
    payload: str
    issued_at: int
    expires_at: int
    signature_algorithm: str = "Ed25519"
    signature: str = ""

    def unsigned_payload(self) -> dict[str, object]:
        value = asdict(self)
        value.pop("signature")
        return value

    def validate(self, *, now: int | None = None) -> None:
        if not all((self.protocol_version, self.message_id, self.request_id, self.sender_node_id, self.target_node_id, self.operation)):
            raise ValueError("missing job request field")
        if self.operation not in _ALLOWED_OPERATIONS:
            raise ValueError("unsupported job operation")
        if self.issued_at < 0 or self.expires_at <= self.issued_at:
            raise ValueError("invalid job validity window")
        if now is not None and not self.issued_at <= int(now) <= self.expires_at:
            raise ValueError("job request expired or not yet valid")
        if self.signature_algorithm != "Ed25519":
            raise ValueError("unsupported signature algorithm")


@dataclass(frozen=True)
class JobResult:
    protocol_version: str
    message_id: str
    request_id: str
    responder_node_id: str
    recipient_node_id: str
    status: str
    payload: str
    completed_at: int
    signature_algorithm: str = "Ed25519"
    signature: str = ""

    def unsigned_payload(self) -> dict[str, object]:
        value = asdict(self)
        value.pop("signature")
        return value

    def validate(self) -> None:
        if not all((self.protocol_version, self.message_id, self.request_id, self.responder_node_id, self.recipient_node_id, self.status)):
            raise ValueError("missing job result field")
        if self.status not in _ALLOWED_STATUS:
            raise ValueError("invalid job result status")
        if self.completed_at < 0:
            raise ValueError("negative completion time")
        if self.signature_algorithm != "Ed25519":
            raise ValueError("unsupported signature algorithm")


def sign_job_request(request: JobRequest, private_key: Ed25519PrivateKey) -> JobRequest:
    request.validate()
    unsigned = JobRequest(**{**asdict(request), "signature": ""})
    signature = private_key.sign(canonical_json(unsigned.unsigned_payload()))
    return JobRequest(**{**asdict(unsigned), "signature": _b64(signature)})


def verify_job_request(request: JobRequest, public_key: Ed25519PublicKey, *, now: int | None = None) -> bool:
    try:
        request.validate(now=now)
        if not request.signature:
            return False
        public_key.verify(_unb64(request.signature), canonical_json(request.unsigned_payload()))
        return True
    except (InvalidSignature, ValueError, UnicodeError, binascii.Error):
        return False


def sign_job_result(result: JobResult, private_key: Ed25519PrivateKey) -> JobResult:
    result.validate()
    unsigned = JobResult(**{**asdict(result), "signature": ""})
    signature = private_key.sign(canonical_json(unsigned.unsigned_payload()))
    return JobResult(**{**asdict(unsigned), "signature": _b64(signature)})


def verify_job_result(result: JobResult, public_key: Ed25519PublicKey) -> bool:
    try:
        result.validate()
        if not result.signature:
            return False
        public_key.verify(_unb64(result.signature), canonical_json(result.unsigned_payload()))
        return True
    except (InvalidSignature, ValueError, UnicodeError, binascii.Error):
        return False


def execute_reference_job(request: JobRequest) -> tuple[str, str]:
    """Reference-safe executor.

    It deliberately supports only fixed operations used by interoperability tests;
    arbitrary remote code execution is outside the MA2A reference scope.
    """
    if request.operation == "PING":
        return "OK", "PONG"
    if request.operation == "ECHO":
        return "OK", request.payload
    return "ERROR", "unsupported_operation"
