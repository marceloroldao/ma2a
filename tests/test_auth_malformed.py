from dataclasses import replace

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from ma2a.auth import HandshakeVerifier, issue_certificate, make_challenge, sign_challenge


NOW = 1_800_000_000


def valid_chain():
    root = Ed25519PrivateKey.generate()
    org = Ed25519PrivateKey.generate()
    device = Ed25519PrivateKey.generate()
    org_cert = issue_certificate(
        issuer_id="ma2a-root",
        issuer_private_key=root,
        subject_id="org:acme",
        subject_kind="organization",
        subject_public_key=org.public_key(),
        not_before=NOW - 60,
        not_after=NOW + 3600,
        capabilities=["delegate"],
    )
    device_cert = issue_certificate(
        issuer_id="org:acme",
        issuer_private_key=org,
        subject_id="device:robot-7",
        subject_kind="device",
        subject_public_key=device.public_key(),
        not_before=NOW - 60,
        not_after=NOW + 600,
        capabilities=["sync:shared"],
    )
    challenge = make_challenge(
        organization_id="org:acme",
        agent_or_device_id="device:robot-7",
        client_nonce="nonce",
        protocol_version="0.1",
        selected_capabilities=["sync:shared"],
        now=NOW,
    )
    signature = sign_challenge(challenge, device)
    return root, org_cert, device_cert, challenge, signature


def verify(root, org_cert, device_cert, challenge, signature):
    return HandshakeVerifier().verify(
        challenge=challenge,
        signature=signature,
        device_certificate=device_cert,
        organization_certificate=org_cert,
        root_public_key=root.public_key(),
        organization_status="ACTIVE",
        device_status="ACTIVE",
        required_capability="sync:shared",
        now=NOW,
    )


def test_malformed_organization_signature_fails_closed():
    root, org_cert, device_cert, challenge, signature = valid_chain()
    bad = replace(org_cert, issuer_signature="%%%not-base64%%%")
    assert verify(root, bad, device_cert, challenge, signature) == (
        False,
        "invalid_organization_certificate",
    )


def test_malformed_device_signature_fails_closed():
    root, org_cert, device_cert, challenge, signature = valid_chain()
    bad = replace(device_cert, issuer_signature="%%%not-base64%%%")
    assert verify(root, org_cert, bad, challenge, signature) == (
        False,
        "invalid_device_certificate",
    )


def test_malformed_handshake_signature_fails_closed():
    root, org_cert, device_cert, challenge, _ = valid_chain()
    assert verify(root, org_cert, device_cert, challenge, "%%%not-base64%%%")== (
        False,
        "invalid_signature",
    )


def test_unknown_certificate_version_is_rejected():
    root, org_cert, device_cert, challenge, signature = valid_chain()
    bad = replace(org_cert, version=99)
    assert verify(root, bad, device_cert, challenge, signature) == (
        False,
        "invalid_organization_certificate",
    )


def test_invalid_device_public_key_encoding_does_not_crash():
    root, org_cert, device_cert, challenge, signature = valid_chain()
    # Signature is necessarily invalid after mutation; verifier must fail closed.
    bad = replace(device_cert, subject_public_key="%%%")
    assert verify(root, org_cert, bad, challenge, signature) == (
        False,
        "invalid_device_certificate",
    )
