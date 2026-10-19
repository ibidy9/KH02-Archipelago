from __future__ import annotations

from dataclasses import dataclass
import json
import os
from pathlib import Path
from typing import Iterable


LEDGER_VERSION = 1


@dataclass(frozen=True)
class ReceivedItem:
    index: int
    item_id: int
    player: int


@dataclass(frozen=True)
class DeliveryKey:
    seed_name: str
    team: int
    slot: int

    @property
    def filename(self) -> str:
        safe_seed = "".join(
            character if character.isalnum() or character in "-_" else "_"
            for character in self.seed_name
        )
        return f"{safe_seed}-t{self.team}-s{self.slot}.json"


class AppliedItemLedger:
    """Durable record of game-side effects, isolated by room and slot.

    An entry is recorded only after its effect has been verified in game. The
    Archipelago server remains authoritative for which items were received;
    this ledger only prevents a reconnect from applying the same effect twice.
    """

    def __init__(self, path: Path, key: DeliveryKey, applied: dict[int, int] | None = None):
        self.path = path
        self.key = key
        self.applied = applied or {}

    @classmethod
    def load(cls, directory: Path, key: DeliveryKey) -> "AppliedItemLedger":
        path = directory / key.filename
        if not path.exists():
            return cls(path, key)
        payload = json.loads(path.read_text(encoding="utf-8"))
        expected = {
            "version": LEDGER_VERSION,
            "seed_name": key.seed_name,
            "team": key.team,
            "slot": key.slot,
        }
        if any(payload.get(field) != value for field, value in expected.items()):
            raise ValueError("Received-item ledger does not match this room and slot")
        applied = {int(index): int(item_id) for index, item_id in payload["applied"].items()}
        return cls(path, key, applied)

    def pending(self, received: Iterable[ReceivedItem]) -> list[ReceivedItem]:
        pending: list[ReceivedItem] = []
        for item in received:
            applied_id = self.applied.get(item.index)
            if applied_id is None:
                pending.append(item)
            elif applied_id != item.item_id:
                raise ValueError(f"Received item index {item.index} changed identity")
        return pending

    def mark_applied(self, item: ReceivedItem) -> None:
        existing = self.applied.get(item.index)
        if existing is not None and existing != item.item_id:
            raise ValueError(f"Received item index {item.index} changed identity")
        self.applied[item.index] = item.item_id
        self._save()

    def _save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "version": LEDGER_VERSION,
            "seed_name": self.key.seed_name,
            "team": self.key.team,
            "slot": self.key.slot,
            "applied": {str(index): item_id for index, item_id in sorted(self.applied.items())},
        }
        temporary = self.path.with_suffix(self.path.suffix + ".tmp")
        temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, self.path)
