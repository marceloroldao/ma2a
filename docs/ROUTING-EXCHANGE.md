# MA2A routing exchange boundary

This document defines the MA2A messages that feed `resolutive-routing` without moving routing logic into the protocol layer.

## CapabilityAdvertisement

A node may advertise routing-relevant capabilities over MA2A. The advertisement is signed with the node identity key and contains only self-owned facts such as compute capacity, current load, models, knowledge domains and supported scopes.

The message deliberately does **not** self-assert:

- trust;
- reputation;
- measured network latency;
- routing score;
- routing priority;
- credit valuation.

Those values are derived by the receiving side, policy layer or routing subsystem. A node must not be able to make itself trusted or preferred simply by advertising a value.

Advertisements are short-lived and sequence-numbered so stale capability state can be rejected by higher-level state tracking.

## FailureNotice

A node may emit a signed failure notice describing an observed failure involving a request and a candidate node. MA2A owns authenticity and transport of the notice. `resolutive-routing` may consume the verified notice as a `FailureEvent` and decide whether to reroute.

MA2A does not decide the replacement route.

## Interface with resolutive-routing

```text
Node
  -> signed CapabilityAdvertisement
  -> MA2A verification / policy context
  -> normalized routing snapshot
  -> resolutive-routing
  -> RouteDecision
  -> MA2A transport executes selected route

transport failure
  -> signed FailureNotice
  -> MA2A verification
  -> routing FailureEvent
  -> RerouteDecision
```

This keeps protocol identity/transport and route selection separate.
