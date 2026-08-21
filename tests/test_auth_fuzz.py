from dataclasses import replace
import random

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from ma2a.auth import HandshakeVerifier, issue_certificate, make_challenge, sign_challenge


NOW = 1_800_000_000


def make_valid():
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
        client_nonce="client-nonce",
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


def mutate_ascii(text: str, rng: random.Random) -> str:
    if not text:
        return "%"
    pos = rng.randrange(len(text))
    replacement = chr(rng.randrange(33, 127))
    if replacement == text[pos]:
        replacement = "%" if text[pos] != "%" else "!"
    return text[:pos] + replacement + text[pos + 1 :]


def test_seeded_certificate_signature_mutations_fail_closed():
    rng = random.Random(20260821)
    for _ in range(100):
        root, org_cert, device_cert, challenge, signature = make_valid()
        if rng.choice([True, False]):
            mutated = replace(org_cert, issuer_signature=mutate_ascii(org_cert.issuer_signature, rng))
            ok, _ = verify(root, mutated, device_cert, challenge, signature)
        else:
            mutated = replace(device_cert, issuer_signature=mutate_ascii(device_cert.issuer_signature, rng))
            ok, _ = verify(root, org_cert, mutated, challenge, signature)
        assert not ok


def test_seeded_transcript_mutations_fail_closed():
    rng = random.Random(20260822)
    fields = ["client_nonce", "server_nonce", "protocol_version", "challenge_id"]
    for _ in range(100):
        root, org_cert, device_cert, challenge, signature = make_valid()
        field = rng.choice(fields)
        mutated = replace(challenge, **{field: mutate_ascii(getattr(challenge, field), rng)})
        ok, _ = verify(root, org_cert, device_cert, mutated, signature)
        assert not ok
