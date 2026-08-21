from ma2a.conflict import DeltaMeta, Resolution, resolve


def d(base, new, clock, org="org", sender="agent", msg="m", **kw):
    return DeltaMeta(base, new, clock, org, sender, msg, **kw)


def test_direct_descendant_is_accepted():
    local = d(0, 1, 1, msg="local")
    incoming = d(1, 2, 2, msg="incoming")
    assert resolve(local, incoming) == Resolution.ACCEPT_INCOMING


def test_replay_is_rejected_before_ordering():
    local = d(0, 1, 1)
    incoming = d(1, 2, 2, replay=True)
    assert resolve(local, incoming) == Resolution.REJECT_REPLAY


def test_scope_violation_is_rejected():
    local = d(0, 1, 1)
    incoming = d(1, 2, 2, scope_allowed=False)
    assert resolve(local, incoming) == Resolution.REJECT_SCOPE


def test_stale_nonconcurrent_update_is_rejected():
    local = d(1, 3, 3)
    incoming = d(1, 2, 2)
    assert resolve(local, incoming) == Resolution.REJECT_STALE


def test_concurrent_candidates_have_stable_total_order():
    a = d(1, 2, 10, org="A", sender="a", msg="1")
    b = d(1, 2, 10, org="B", sender="a", msg="1")
    # B sorts after A under the canonical byte ordering used by v0.1.
    assert resolve(a, b) == Resolution.ACCEPT_INCOMING
    assert resolve(b, a) == Resolution.KEEP_LOCAL


def test_future_base_is_not_invented_or_merged():
    local = d(0, 1, 1)
    incoming = d(5, 6, 6)
    assert resolve(local, incoming) == Resolution.REJECT_UNRESOLVED
