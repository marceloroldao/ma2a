from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from typing import Callable

from .wire import ReferenceDelta, decode_reference_delta, encode_reference_delta


@dataclass(frozen=True)
class RouteRecord:
    trajectory_address: str
    condition_key: str
    result: str
    version: int
    scope: str = "shared"


class ResolutiveRouteMemory:
    """Minimal reference bridge between resolutive route memory and MA2A.

    This deliberately models only the narrow mechanism needed for the
    RM<->MA2A integration experiment:

    condition -> stable route identity -> deterministic local hit
             -> external fallback on miss -> validated registration
             -> synchronization of that same route identity.

    It is not a semantic-memory implementation and does not claim that
    arbitrary natural-language discovery is O(1).
    """

    def __init__(self) -> None:
        self._by_condition: dict[str, str] = {}
        self._by_address: dict[str, RouteRecord] = {}
        self.external_resolution_calls = 0

    @staticmethod
    def condition_key(condition: str) -> str:
        normalized = " ".join(condition.strip().lower().split())
        return hashlib.sha256(normalized.encode("utf-8")).hexdigest()

    @staticmethod
    def derive_address(condition_key: str, result: str) -> str:
        material = f"rm-route-v1|{condition_key}|{result}".encode("utf-8")
        return "rm:" + hashlib.sha256(material).hexdigest()

    def resolve(
        self,
        condition: str,
        external_resolver: Callable[[str], str] | None = None,
        validator: Callable[[str], bool] | None = None,
    ) -> tuple[str, bool, str]:
        """Return (result, local_hit, trajectory_address)."""
        key = self.condition_key(condition)
        address = self._by_condition.get(key)
        if address is not None:
            record = self._by_address[address]
            return record.result, True, record.trajectory_address

        if external_resolver is None:
            raise KeyError("unknown route and no external resolver supplied")

        self.external_resolution_calls += 1
        result = external_resolver(condition)
        if validator is not None and not validator(result):
            raise ValueError("external result rejected by validator")

        address = self.derive_address(key, result)
        record = RouteRecord(
            trajectory_address=address,
            condition_key=key,
            result=result,
            version=1,
            scope="shared",
        )
        self._by_condition[key] = address
        self._by_address[address] = record
        return result, False, address

    def export_delta(
        self,
        trajectory_address: str,
        *,
        sender_id: str,
        organization_id: str,
        message_id: str,
        logical_clock: int,
    ) -> bytes:
        record = self._by_address[trajectory_address]
        payload = json.dumps(
            {
                "condition_key": record.condition_key,
                "result": record.result,
                "version": record.version,
            },
            sort_keys=True,
            separators=(",", ":"),
        )
        digest = hashlib.sha256(payload.encode("utf-8")).hexdigest()
        delta = ReferenceDelta(
            protocol_version="0.1",
            message_id=message_id,
            sender_id=sender_id,
            organization_id=organization_id,
            trajectory_address=record.trajectory_address,
            base_version=0,
            new_version=record.version,
            operation="SET",
            scope=record.scope,
            payload_or_reference=payload,
            integrity_digest=digest,
            logical_clock=logical_clock,
            signature="reference-test-signature",
        )
        return encode_reference_delta(delta)

    def import_delta(self, encoded: bytes) -> RouteRecord:
        delta = decode_reference_delta(encoded)
        payload_bytes = delta.payload_or_reference.encode("utf-8")
        digest = hashlib.sha256(payload_bytes).hexdigest()
        if digest != delta.integrity_digest:
            raise ValueError("integrity mismatch")
        if delta.scope not in {"shared", "global"}:
            raise ValueError("route is not eligible for cross-agent import")

        raw = json.loads(delta.payload_or_reference)
        condition_key = raw["condition_key"]
        result = raw["result"]
        version = int(raw["version"])
        expected_address = self.derive_address(condition_key, result)
        if expected_address != delta.trajectory_address:
            raise ValueError("route identity mismatch")

        record = RouteRecord(
            trajectory_address=delta.trajectory_address,
            condition_key=condition_key,
            result=result,
            version=version,
            scope=delta.scope,
        )
        self._by_address[record.trajectory_address] = record
        self._by_condition[condition_key] = record.trajectory_address
        return record
