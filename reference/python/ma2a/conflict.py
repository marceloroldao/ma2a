from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class Resolution(str, Enum):
    ACCEPT_INCOMING = "accept_incoming"
    KEEP_LOCAL = "keep_local"
    REJECT_REPLAY = "reject_replay"
    REJECT_STALE = "reject_stale"
    REJECT_SCOPE = "reject_scope"
    REJECT_UNRESOLVED = "reject_unresolved"


@dataclass(frozen=True)
class DeltaMeta:
    base_version: int
    new_version: int
    logical_clock: int
    organization_id: str
    sender_id: str
    message_id: str
    scope_allowed: bool = True
    replay: bool = False

    def ordering_key(self) -> tuple[int, int, bytes, bytes, bytes]:
        return (
            self.new_version,
            self.logical_clock,
            self.organization_id.encode("utf-8"),
            self.sender_id.encode("utf-8"),
            self.message_id.encode("utf-8"),
        )


def resolve(local: DeltaMeta, incoming: DeltaMeta) -> Resolution:
    """Reference resolver for MA2A v0.1 deterministic ordering.

    This function resolves only metadata-level conflicts covered by the v0.1
    default rule. Domain-specific semantic merges are intentionally excluded.
    """

    if incoming.replay:
        return Resolution.REJECT_REPLAY

    if not incoming.scope_allowed:
        return Resolution.REJECT_SCOPE

    if incoming.base_version == local.new_version:
        return Resolution.ACCEPT_INCOMING

    if incoming.base_version < local.new_version:
        # Same-generation/concurrent candidates may be deterministically ordered
        # only when they represent comparable descendants. The simple reference
        # model treats equal new_version as concurrent candidates.
        if incoming.new_version == local.new_version:
            return (
                Resolution.ACCEPT_INCOMING
                if incoming.ordering_key() > local.ordering_key()
                else Resolution.KEEP_LOCAL
            )
        return Resolution.REJECT_STALE

    # A future-base mutation cannot be safely invented/merged by this resolver.
    return Resolution.REJECT_UNRESOLVED
