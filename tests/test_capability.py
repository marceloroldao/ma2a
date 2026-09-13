from dataclasses import replace

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from ma2a.capability import (
    CapabilityAdvertisement,
    FailureNotice,
    sign_capability_advertisement,
    sign_failure_notice,
    verify_capability_advertisement,
    verify_failure_notice,
)


def test_signed_capability_advertisement_verifies():
    private = Ed25519PrivateKey.generate()
    advertisement = CapabilityAdvertisement(
        protocol_version="0.2",
        message_id="cap-1",
        node_id="node-a",
        organization_id="org-1",
        sequence=7,
        issued_at=100,
        expires_at=200,
        available=True,
        compute_capacity=64.0,
        current_load=0.25,
        models=("qwen_7b", "small_llm"),
        memory_domains=("electronics",),
        supported_scopes=("ORGANIZATION", "PUBLIC"),
    )
    signed = sign_capability_advertisement(advertisement, private)
    assert verify_capability_advertisement(signed, private.public_key(), now=150)


def test_capability_tampering_is_detected():
    private = Ed25519PrivateKey.generate()
    advertisement = CapabilityAdvertisement(
        protocol_version="0.2",
        message_id="cap-2",
        node_id="node-a",
        organization_id="org-1",
        sequence=1,
        issued_at=100,
        expires_at=200,
        available=True,
        compute_capacity=64.0,
        current_load=0.25,
        models=("small_llm",),
        memory_domains=(),
        supported_scopes=("PUBLIC",),
    )
    signed = sign_capability_advertisement(advertisement, private)
    tampered = replace(signed, current_load=0.0)
    assert not verify_capability_advertisement(tampered, private.public_key(), now=150)


def test_expired_capability_is_rejected():
    private = Ed25519PrivateKey.generate()
    advertisement = CapabilityAdvertisement(
        protocol_version="0.2",
        message_id="cap-3",
        node_id="node-a",
        organization_id="org-1",
        sequence=1,
        issued_at=100,
        expires_at=200,
        available=True,
        compute_capacity=32.0,
        current_load=0.5,
        models=(),
        memory_domains=(),
        supported_scopes=("PUBLIC",),
    )
    signed = sign_capability_advertisement(advertisement, private)
    assert not verify_capability_advertisement(signed, private.public_key(), now=201)


def test_signed_failure_notice_verifies_and_tampering_fails():
    private = Ed25519PrivateKey.generate()
    notice = FailureNotice(
        protocol_version="0.2",
        message_id="fail-1",
        request_id="req-9",
        reporting_node_id="node-a",
        failed_node_id="node-b",
        reason="transport_timeout",
        observed_at=123,
    )
    signed = sign_failure_notice(notice, private)
    assert verify_failure_notice(signed, private.public_key())
    assert not verify_failure_notice(replace(signed, failed_node_id="node-c"), private.public_key())


def test_trust_and_latency_are_not_self_advertised():
    fields = CapabilityAdvertisement.__dataclass_fields__
    assert "trusted" not in fields
    assert "latency_ms" not in fields
    assert "reputation" not in fields
