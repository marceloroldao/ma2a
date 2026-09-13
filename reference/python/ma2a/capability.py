from __future__ import annotations

from dataclasses import asdict, dataclass
import base64
import binascii

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey

from .auth import canonical_json


_ALLOWED_SCOPES = {"LOCAL_ONLY", "PRIVATE", "ORGANIZATION", "PUBLIC"}


def _b64(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii")


def _unb64(text: str) -> bytes:
    return base64.b64decode(text.encode("ascii"), altchars=b"-_", validate=True)


@dataclass(frozen=True)
class CapabilityAdvertisement:
    protocol_version: str
    message_id: str
    node_id: str
    organization_id: str | None
    sequence: int
    issued_at: int
    expires_at: int
    available: bool
    compute_capacity: float
    current_load: float
    models: tuple[str, ...]
    memory_domains: tuple[str, ...]
    supported_scopes: tuple[str, ...]
    signature_algorithm: str = "Ed25519"
    signature: str = ""

    def unsigned_payload(self) -> dict[str, object]:
        value = asdict(self)
        value.pop("signature")
        value["models"] = list(self.models)
        value["memory_domains"] = list(self.memory_domains)
        value["supported_scopes"] = list(self.supported_scopes)
        return value

    def validate(self, *, now: int | None = None) -> None:
        if not self.protocol_version or not self.message_id or not self.node_id:
            raise ValueError("missing capability identity field")
        if self.sequence < 0:
            raise ValueError("negative capability sequence")
        if self.issued_at < 0 or self.expires_at <= self.issued_at:
            raise ValueError("invalid capability validity window")
        if now is not None and not self.issued_at <= int(now) <= self.expires_at:
            raise ValueError("capability advertisement expired or not yet valid")
        if self.compute_capacity < 0:
            raise ValueError("negative compute capacity")
        if not 0.0 <= self.current_load <= 1.0:
            raise ValueError("current_load must be between 0 and 1")
        if not self.supported_scopes:
            raise ValueError("supported_scopes must not be empty")
        if any(scope not in _ALLOWED_SCOPES for scope in self.supported_scopes):
            raise ValueError("invalid supported scope")
        if self.signature_algorithm != "Ed25519":
            raise ValueError("unsupported signature algorithm")


def sign_capability_advertisement(
    advertisement: CapabilityAdvertisement,
    private_key: Ed25519PrivateKey,
) -> CapabilityAdvertisement:
    advertisement.validate()
    unsigned = CapabilityAdvertisement(
        protocol_version=advertisement.protocol_version,
        message_id=advertisement.message_id,
        node_id=advertisement.node_id,
        organization_id=advertisement.organization_id,
        sequence=advertisement.sequence,
        issued_at=advertisement.issued_at,
        expires_at=advertisement.expires_at,
        available=advertisement.available,
        compute_capacity=advertisement.compute_capacity,
        current_load=advertisement.current_load,
        models=tuple(sorted(set(advertisement.models))),
        memory_domains=tuple(sorted(set(advertisement.memory_domains))),
        supported_scopes=tuple(sorted(set(advertisement.supported_scopes))),
        signature_algorithm="Ed25519",
        signature="",
    )
    signature = private_key.sign(canonical_json(unsigned.unsigned_payload()))
    return CapabilityAdvertisement(**{**asdict(unsigned), "signature": _b64(signature)})


def verify_capability_advertisement(
    advertisement: CapabilityAdvertisement,
    public_key: Ed25519PublicKey,
    *,
    now: int | None = None,
) -> bool:
    try:
        advertisement.validate(now=now)
        if not advertisement.signature:
            return False
        public_key.verify(
            _unb64(advertisement.signature),
            canonical_json(advertisement.unsigned_payload()),
        )
        return True
    except (InvalidSignature, ValueError, UnicodeError, binascii.Error):
        return False


@dataclass(frozen=True)
class FailureNotice:
    protocol_version: str
    message_id: str
    request_id: str
    reporting_node_id: str
    failed_node_id: str
    reason: str
    observed_at: int
    signature_algorithm: str = "Ed25519"
    signature: str = ""

    def unsigned_payload(self) -> dict[str, object]:
        value = asdict(self)
        value.pop("signature")
        return value

    def validate(self) -> None:
        if not all((self.protocol_version, self.message_id, self.request_id, self.reporting_node_id, self.failed_node_id, self.reason)):
            raise ValueError("missing failure notice field")
        if self.observed_at < 0:
            raise ValueError("negative observation time")
        if self.signature_algorithm != "Ed25519":
            raise ValueError("unsupported signature algorithm")


def sign_failure_notice(notice: FailureNotice, private_key: Ed25519PrivateKey) -> FailureNotice:
    notice.validate()
    unsigned = FailureNotice(**{**asdict(notice), "signature": ""})
    signature = private_key.sign(canonical_json(unsigned.unsigned_payload()))
    return FailureNotice(**{**asdict(unsigned), "signature": _b64(signature)})


def verify_failure_notice(notice: FailureNotice, public_key: Ed25519PublicKey) -> bool:
    try:
        notice.validate()
        if not notice.signature:
            return False
        public_key.verify(_unb64(notice.signature), canonical_json(notice.unsigned_payload()))
        return True
    except (InvalidSignature, ValueError, UnicodeError, binascii.Error):
        return False
