from __future__ import annotations

from dataclasses import dataclass, asdict
from datetime import datetime, timezone
import base64
import json
import secrets
from typing import Iterable

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric.ed25519 import (
    Ed25519PrivateKey,
    Ed25519PublicKey,
)


def _b64(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii")


def _unb64(text: str) -> bytes:
    return base64.urlsafe_b64decode(text.encode("ascii"))


def canonical_json(value: dict) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


@dataclass(frozen=True)
class Certificate:
    version: int
    serial: str
    subject_id: str
    subject_kind: str
    subject_public_key: str
    issuer_id: str
    not_before: int
    not_after: int
    capabilities: tuple[str, ...]
    signature_algorithm: str
    issuer_signature: str

    def unsigned_payload(self) -> dict:
        value = asdict(self)
        value.pop("issuer_signature")
        value["capabilities"] = list(self.capabilities)
        return value


def issue_certificate(
    *,
    issuer_id: str,
    issuer_private_key: Ed25519PrivateKey,
    subject_id: str,
    subject_kind: str,
    subject_public_key: Ed25519PublicKey,
    not_before: int,
    not_after: int,
    capabilities: Iterable[str],
) -> Certificate:
    public_raw = subject_public_key.public_bytes_raw()
    capability_list = sorted(set(capabilities))
    unsigned = {
        "version": 1,
        "serial": secrets.token_hex(16),
        "subject_id": subject_id,
        "subject_kind": subject_kind,
        "subject_public_key": _b64(public_raw),
        "issuer_id": issuer_id,
        "not_before": int(not_before),
        "not_after": int(not_after),
        "capabilities": capability_list,
        "signature_algorithm": "Ed25519",
    }
    signature = issuer_private_key.sign(canonical_json(unsigned))
    return Certificate(
        version=unsigned["version"],
        serial=unsigned["serial"],
        subject_id=unsigned["subject_id"],
        subject_kind=unsigned["subject_kind"],
        subject_public_key=unsigned["subject_public_key"],
        issuer_id=unsigned["issuer_id"],
        not_before=unsigned["not_before"],
        not_after=unsigned["not_after"],
        capabilities=tuple(capability_list),
        signature_algorithm=unsigned["signature_algorithm"],
        issuer_signature=_b64(signature),
    )


def verify_certificate(
    cert: Certificate,
    issuer_public_key: Ed25519PublicKey,
    *,
    now: int | None = None,
) -> bool:
    now = int(datetime.now(timezone.utc).timestamp()) if now is None else int(now)
    if cert.signature_algorithm != "Ed25519":
        return False
    if not cert.not_before <= now <= cert.not_after:
        return False
    try:
        issuer_public_key.verify(_unb64(cert.issuer_signature), canonical_json(cert.unsigned_payload()))
        return True
    except (InvalidSignature, ValueError):
        return False


@dataclass(frozen=True)
class Challenge:
    challenge_id: str
    client_nonce: str
    server_nonce: str
    organization_id: str
    agent_or_device_id: str
    protocol_version: str
    selected_capabilities: tuple[str, ...]
    issued_at: int
    expires_at: int

    def transcript(self) -> bytes:
        return canonical_json(
            {
                "challenge_id": self.challenge_id,
                "client_nonce": self.client_nonce,
                "server_nonce": self.server_nonce,
                "organization_id": self.organization_id,
                "agent_or_device_id": self.agent_or_device_id,
                "protocol_version": self.protocol_version,
                "selected_capabilities": list(self.selected_capabilities),
            }
        )


def make_challenge(
    *,
    organization_id: str,
    agent_or_device_id: str,
    client_nonce: str,
    protocol_version: str,
    selected_capabilities: Iterable[str],
    now: int,
    ttl_seconds: int = 30,
) -> Challenge:
    return Challenge(
        challenge_id=secrets.token_hex(16),
        client_nonce=client_nonce,
        server_nonce=secrets.token_hex(32),
        organization_id=organization_id,
        agent_or_device_id=agent_or_device_id,
        protocol_version=protocol_version,
        selected_capabilities=tuple(sorted(set(selected_capabilities))),
        issued_at=int(now),
        expires_at=int(now) + int(ttl_seconds),
    )


def sign_challenge(challenge: Challenge, private_key: Ed25519PrivateKey) -> str:
    return _b64(private_key.sign(challenge.transcript()))


class HandshakeVerifier:
    def __init__(self) -> None:
        self._consumed: set[str] = set()

    def verify(
        self,
        *,
        challenge: Challenge,
        signature: str,
        device_certificate: Certificate,
        organization_certificate: Certificate,
        root_public_key: Ed25519PublicKey,
        organization_status: str,
        required_capability: str | None,
        now: int,
    ) -> tuple[bool, str]:
        if challenge.challenge_id in self._consumed:
            return False, "replay"
        if not challenge.issued_at <= now <= challenge.expires_at:
            return False, "expired_challenge"
        if organization_status != "ACTIVE":
            return False, "inactive_organization"
        if organization_certificate.subject_kind != "organization":
            return False, "invalid_organization_certificate"
        if device_certificate.subject_kind not in {"device", "agent"}:
            return False, "invalid_device_certificate"
        if organization_certificate.subject_id != challenge.organization_id:
            return False, "organization_mismatch"
        if device_certificate.subject_id != challenge.agent_or_device_id:
            return False, "subject_mismatch"
        if not verify_certificate(organization_certificate, root_public_key, now=now):
            return False, "invalid_organization_certificate"

        organization_public_key = Ed25519PublicKey.from_public_bytes(
            _unb64(organization_certificate.subject_public_key)
        )
        if device_certificate.issuer_id != organization_certificate.subject_id:
            return False, "invalid_delegation"
        if not verify_certificate(device_certificate, organization_public_key, now=now):
            return False, "invalid_device_certificate"
        if required_capability and required_capability not in device_certificate.capabilities:
            return False, "unauthorized_capability"

        device_public_key = Ed25519PublicKey.from_public_bytes(
            _unb64(device_certificate.subject_public_key)
        )
        try:
            device_public_key.verify(_unb64(signature), challenge.transcript())
        except (InvalidSignature, ValueError):
            return False, "invalid_signature"

        self._consumed.add(challenge.challenge_id)
        return True, "ok"
