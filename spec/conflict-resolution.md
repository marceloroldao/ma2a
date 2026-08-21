# MA2A Deterministic Conflict Resolution — v0.1 draft

## Goal

For identical accepted inputs and the same protocol/policy version, conforming implementations must reach the same conflict classification and resolution outcome.

## Conflict classes

- `STALE`: `base_version` is older than the currently accepted version and the operation is not valid as a descendant.
- `REPLAY`: `message_id` or equivalent anti-replay token has already been accepted/observed within the replay window.
- `CONCURRENT`: two valid descendants originate from the same base without a predefined causal order.
- `SCOPE`: requested mutation or relay violates privacy/authorization policy.
- `INTEGRITY`: signature/digest/canonicalization validation fails.
- `SEMANTIC_POLICY`: both mutations are structurally valid but application policy requires a domain-specific deterministic resolver.

## Default ordering tuple

Where a conflict can be safely resolved through deterministic total ordering, v0.1 defines the comparison tuple:

```text
(new_version, logical_clock, organization_id, sender_id, message_id)
```

All fields must use canonical byte ordering. The same tuple must produce the same winner on every conforming node.

This ordering is a convergence mechanism, not a claim that the winning mutation is semantically preferable.

## Safety rule

A resolver MUST reject rather than invent a merge when the active policy does not define a deterministic outcome for the conflict class.

## Pseudocode

```text
RESOLVE(local, incoming, policy_version):
    validate(incoming)

    if replay(incoming):
        return REJECT(REPLAY)

    if violates_scope(incoming):
        return REJECT(SCOPE)

    if incoming.base_version == local.version:
        return ACCEPT(incoming)

    if incoming.base_version < local.version and not concurrent(local, incoming):
        return REJECT(STALE)

    if concurrent(local, incoming):
        policy := resolver(policy_version, incoming.operation)
        if policy is undefined:
            return REJECT(UNRESOLVED_CONFLICT)
        return policy(local, incoming)
```

## Convergence property to test

Given the same finite set of valid deltas `D`, the same initial state `S0`, and the same policy version `P`, all replicas must converge to the same accepted state after processing all causally admissible inputs, independent of arrival order where the specified resolver permits reordering.

This property is a test target for v0.1 and is not considered proven by this document alone.
