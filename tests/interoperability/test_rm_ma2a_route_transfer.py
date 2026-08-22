from ma2a.rm_bridge import ResolutiveRouteMemory


def test_agent_a_learns_and_agent_b_reuses_same_route_without_fallback():
    agent_a = ResolutiveRouteMemory()
    agent_b = ResolutiveRouteMemory()

    resolver_calls = {"a": 0, "b": 0}

    def resolver_a(condition: str) -> str:
        resolver_calls["a"] += 1
        assert condition == "sensor-voltage-high"
        return "reduce_pwm_duty"

    def resolver_b(_: str) -> str:
        resolver_calls["b"] += 1
        return "should-not-be-called"

    # 1) Agent A encounters an unknown condition and invokes external resolution.
    result_1, hit_1, address_1 = agent_a.resolve(
        "sensor-voltage-high",
        external_resolver=resolver_a,
        validator=lambda result: result == "reduce_pwm_duty",
    )
    assert result_1 == "reduce_pwm_duty"
    assert hit_1 is False
    assert resolver_calls["a"] == 1

    # 2) The same condition is now a deterministic local route hit in A.
    result_2, hit_2, address_2 = agent_a.resolve(
        "sensor-voltage-high",
        external_resolver=resolver_a,
    )
    assert result_2 == result_1
    assert hit_2 is True
    assert address_2 == address_1
    assert resolver_calls["a"] == 1

    # 3) A exports the learned route through the MA2A ReferenceDelta envelope.
    encoded_delta = agent_a.export_delta(
        address_1,
        sender_id="agent-a",
        organization_id="org-example",
        message_id="msg-0001",
        logical_clock=1,
    )

    # 4) B imports that exact addressed route identity.
    imported = agent_b.import_delta(encoded_delta)
    assert imported.trajectory_address == address_1
    assert imported.result == "reduce_pwm_duty"

    # 5) B resolves the same condition locally; its external resolver is never called.
    result_b, hit_b, address_b = agent_b.resolve(
        "sensor-voltage-high",
        external_resolver=resolver_b,
    )
    assert result_b == "reduce_pwm_duty"
    assert hit_b is True
    assert address_b == address_1
    assert resolver_calls["b"] == 0
    assert agent_b.external_resolution_calls == 0


def test_import_rejects_tampered_payload():
    agent_a = ResolutiveRouteMemory()
    agent_b = ResolutiveRouteMemory()

    _, _, address = agent_a.resolve(
        "known-condition",
        external_resolver=lambda _: "known-action",
    )
    encoded = agent_a.export_delta(
        address,
        sender_id="agent-a",
        organization_id="org-example",
        message_id="msg-0002",
        logical_clock=2,
    )

    tampered = encoded.replace(b"known-action", b"wrong-action")

    try:
        agent_b.import_delta(tampered)
    except ValueError as exc:
        assert "integrity mismatch" in str(exc)
    else:
        raise AssertionError("tampered payload must be rejected")
