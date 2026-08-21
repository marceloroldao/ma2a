from dataclasses import replace

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from ma2a.auth import HandshakeVerifier, issue_certificate, make_challenge, sign_challenge


def fixture_chain(now=1_800_000_000):
    root_private = Ed25519PrivateKey.generate()
    organization_private = Ed25519PrivateKey.generate()
    device_private = Ed25519PrivateKey.generate()

    organization_certificate = issue_certificate(
        issuer_id="ma2a-root",
        issuer_private_key=root_private,
        subject_id="org:acme",
        subject_kind="organization",
        subject_public_key=organization_private.public_key(),
        not_before=now - 60,
        not_after=now + 3600,
        capabilities=["delegate"],
    )
    device_certificate = issue_certificate(
        issuer_id="org:acme",
        issuer_private_key=organization_private,
        subject_id="device:robot-7",
        subject_kind="device",
        subject_public_key=device_private.public_key(),
        not_before=now - 60,
        not_after=now + 600,
        capabilities=["sync:shared"],
    )
    return root_private, organization_private, device_private, organization_certificate, device_certificate


def make_valid(now=1_800_000_000):
    root, org, device, org_cert, device_cert = fixture_chain(now)
    challenge = make_challenge(
        organization_id="org:acme",
        agent_or_device_id="device:robot-7",
        client_nonce="client-nonce",
        protocol_version="0.1",
        selected_capabilities=["sync:shared"],
        now=now,
    )
    signature = sign_challenge(challenge, device)
    return root, org, device, org_cert, device_cert, challenge, signature


def verify_case(
    root,
    org_cert,
    device_cert,
    challenge,
    signature,
    *,
    now,
    organization_status="ACTIVE",
    device_status="ACTIVE",
    required_capability="sync:shared",
):
    return HandshakeVerifier().verify(
        challenge=challenge,
        signature=signature,
        device_certificate=device_cert,
        organization_certificate=org_cert,
        root_public_key=root.public_key(),
        organization_status=organization_status,
        required_capability=required_capability,
        now=now,
        device_status=device_status,
    )


def test_valid_handshake():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    assert verify_case(root, org_cert, device_cert, challenge, signature, now=now) == (True, "ok")


def test_replay_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    verifier = HandshakeVerifier()
    assert verifier.verify(
        challenge=challenge,
        signature=signature,
        device_certificate=device_cert,
        organization_certificate=org_cert,
        root_public_key=root.public_key(),
        organization_status="ACTIVE",
        required_capability="sync:shared",
        now=now,
    )[0]
    assert verifier.verify(
        challenge=challenge,
        signature=signature,
        device_certificate=device_cert,
        organization_certificate=org_cert,
        root_public_key=root.public_key(),
        organization_status="ACTIVE",
        required_capability="sync:shared",
        now=now,
    ) == (False, "replay")


def test_wrong_signing_key_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, _ = make_valid(now)
    attacker = Ed25519PrivateKey.generate()
    signature = sign_challenge(challenge, attacker)
    assert verify_case(root, org_cert, device_cert, challenge, signature, now=now) == (False, "invalid_signature")


def test_expired_challenge_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    assert verify_case(root, org_cert, device_cert, challenge, signature, now=challenge.expires_at + 1) == (False, "expired_challenge")


def test_revoked_organization_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    assert verify_case(root, org_cert, device_cert, challenge, signature, now=now, organization_status="REVOKED") == (False, "inactive_organization")


def test_revoked_device_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    assert verify_case(root, org_cert, device_cert, challenge, signature, now=now, device_status="REVOKED") == (False, "inactive_device")


def test_missing_capability_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    assert verify_case(root, org_cert, device_cert, challenge, signature, now=now, required_capability="publish:global") == (False, "unauthorized_capability")


def test_wrong_root_is_rejected():
    now = 1_800_000_000
    _, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    wrong_root = Ed25519PrivateKey.generate()
    assert HandshakeVerifier().verify(
        challenge=challenge,
        signature=signature,
        device_certificate=device_cert,
        organization_certificate=org_cert,
        root_public_key=wrong_root.public_key(),
        organization_status="ACTIVE",
        required_capability="sync:shared",
        now=now,
    ) == (False, "invalid_organization_certificate")


def test_tampered_device_certificate_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    tampered = replace(device_cert, capabilities=("sync:shared", "publish:global"))
    assert verify_case(root, org_cert, tampered, challenge, signature, now=now) == (False, "invalid_device_certificate")


def test_wrong_delegation_issuer_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    tampered = replace(device_cert, issuer_id="org:other")
    assert verify_case(root, org_cert, tampered, challenge, signature, now=now) == (False, "invalid_delegation")


def test_tampered_transcript_nonce_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    tampered_challenge = replace(challenge, client_nonce="different-client-nonce")
    assert verify_case(root, org_cert, device_cert, tampered_challenge, signature, now=now) == (False, "invalid_signature")


def test_tampered_selected_capabilities_is_rejected():
    now = 1_800_000_000
    root, _, _, org_cert, device_cert, challenge, signature = make_valid(now)
    tampered_challenge = replace(challenge, selected_capabilities=("publish:global",))
    assert verify_case(root, org_cert, device_cert, tampered_challenge, signature, now=now) == (False, "invalid_signature")


def test_expired_device_certificate_is_rejected():
    now = 1_800_000_000
    root, org, device, org_cert, _ = fixture_chain(now)
    expired_device_cert = issue_certificate(
        issuer_id="org:acme",
        issuer_private_key=org,
        subject_id="device:robot-7",
        subject_kind="device",
        subject_public_key=device.public_key(),
        not_before=now - 120,
        not_after=now - 1,
        capabilities=["sync:shared"],
    )
    challenge = make_challenge(
        organization_id="org:acme",
        agent_or_device_id="device:robot-7",
        client_nonce="client-nonce",
        protocol_version="0.1",
        selected_capabilities=["sync:shared"],
        now=now,
    )
    signature = sign_challenge(challenge, device)
    assert verify_case(root, org_cert, expired_device_cert, challenge, signature, now=now) == (False, "invalid_device_certificate")


def test_expired_organization_certificate_is_rejected():
    now = 1_800_000_000
    root = Ed25519PrivateKey.generate()
    org = Ed25519PrivateKey.generate()
    device = Ed25519PrivateKey.generate()
    expired_org_cert = issue_certificate(
        issuer_id="ma2a-root",
        issuer_private_key=root,
        subject_id="org:acme",
        subject_kind="organization",
        subject_public_key=org.public_key(),
        not_before=now - 120,
        not_after=now - 1,
        capabilities=["delegate"],
    )
    device_cert = issue_certificate(
        issuer_id="org:acme",
        issuer_private_key=org,
        subject_id="device:robot-7",
        subject_kind="device",
        subject_public_key=device.public_key(),
        not_before=now - 60,
        not_after=now + 600,
        capabilities=["sync:shared"],
    )
    challenge = make_challenge(
        organization_id="org:acme",
        agent_or_device_id="device:robot-7",
        client_nonce="client-nonce",
        protocol_version="0.1",
        selected_capabilities=["sync:shared"],
        now=now,
    )
    signature = sign_challenge(challenge, device)
    assert verify_case(root, expired_org_cert, device_cert, challenge, signature, now=now) == (False, "invalid_organization_certificate")
