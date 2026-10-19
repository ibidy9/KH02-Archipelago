from __future__ import annotations

from dataclasses import asdict, dataclass
from enum import Enum
import json
import os
from pathlib import Path


HP_PER_ITEM = 5
MP_PER_ITEM = 10
STATE_VERSION = 1
MAX_HP_ITEM_ID = 0x02020B
MAX_MP_ITEM_ID = 0x02020C


@dataclass(frozen=True)
class LiveStatIdentity:
    process_token: str
    provider_address: int
    attributes_address: int


@dataclass(frozen=True)
class StatApplicationState:
    identity: LiveStatIdentity
    native_max_hp: int
    native_max_mp: int
    applied_hp_items: int
    applied_mp_items: int

    @property
    def target_max_hp(self) -> int:
        return self.native_max_hp + self.applied_hp_items * HP_PER_ITEM

    @property
    def target_max_mp(self) -> int:
        return self.native_max_mp + self.applied_mp_items * MP_PER_ITEM


class SyncAction(Enum):
    NO_CHANGE = "no_change"
    APPLY = "apply"
    REFUSE = "refuse"


@dataclass(frozen=True)
class StatSyncPlan:
    action: SyncAction
    state: StatApplicationState | None
    reason: str


def plan_stat_sync(
    *,
    identity: LiveStatIdentity,
    observed_max_hp: int,
    observed_max_mp: int,
    received_hp_items: int,
    received_mp_items: int,
    confirmed_hp_items: int,
    confirmed_mp_items: int,
    previous: StatApplicationState | None,
) -> StatSyncPlan:
    counts = (received_hp_items, received_mp_items, confirmed_hp_items, confirmed_mp_items)
    if any(value < 0 for value in counts):
        return StatSyncPlan(SyncAction.REFUSE, previous, "Negative item count")
    if confirmed_hp_items > received_hp_items or confirmed_mp_items > received_mp_items:
        return StatSyncPlan(SyncAction.REFUSE, previous, "Ledger exceeds received items")

    same_process = previous is not None and previous.identity.process_token == identity.process_token
    same_object = (
        same_process
        and previous is not None
        and previous.identity.attributes_address == identity.attributes_address
    )
    if previous is None and (confirmed_hp_items or confirmed_mp_items):
        return StatSyncPlan(
            SyncAction.REFUSE,
            None,
            "Applied-item ledger exists but stat session state is missing",
        )

    if same_object:
        assert previous is not None
        expected = (previous.target_max_hp, previous.target_max_mp)
        native = (previous.native_max_hp, previous.native_max_mp)
        confirmed_target = (
            previous.native_max_hp + confirmed_hp_items * HP_PER_ITEM,
            previous.native_max_mp + confirmed_mp_items * MP_PER_ITEM,
        )
        observed = (observed_max_hp, observed_max_mp)
        # Native covers an in-place load reset. Confirmed target covers a stop
        # after durable intent was saved but before a newly received item was
        # written and verified in the delivery ledger.
        if observed not in (expected, native, confirmed_target):
            return StatSyncPlan(
                SyncAction.REFUSE,
                previous,
                f"Observed stats {observed} match neither target {expected}, "
                f"confirmed target {confirmed_target}, nor native {native}",
            )
        native_hp, native_mp = native
    else:
        if (
            previous is not None
            and same_process
            and (observed_max_hp, observed_max_mp)
            == (previous.target_max_hp, previous.target_max_mp)
            and (previous.applied_hp_items or previous.applied_mp_items)
        ):
            return StatSyncPlan(
                SyncAction.REFUSE,
                previous,
                "New attributes object already equals the prior AP target",
            )
        # A newly created gameplay object starts from the game's native values.
        native_hp, native_mp = observed_max_hp, observed_max_mp

    state = StatApplicationState(
        identity=identity,
        native_max_hp=native_hp,
        native_max_mp=native_mp,
        applied_hp_items=received_hp_items,
        applied_mp_items=received_mp_items,
    )
    target = (state.target_max_hp, state.target_max_mp)
    observed = (observed_max_hp, observed_max_mp)
    action = SyncAction.NO_CHANGE if observed == target else SyncAction.APPLY
    return StatSyncPlan(action, state, "Stats already match" if action is SyncAction.NO_CHANGE else "Apply absolute targets")


def load_stat_state(path: Path) -> StatApplicationState | None:
    if not path.exists():
        return None
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("version") != STATE_VERSION:
        raise ValueError("Unsupported stat application state version")
    identity = LiveStatIdentity(**payload["identity"])
    return StatApplicationState(
        identity=identity,
        native_max_hp=int(payload["native_max_hp"]),
        native_max_mp=int(payload["native_max_mp"]),
        applied_hp_items=int(payload["applied_hp_items"]),
        applied_mp_items=int(payload["applied_mp_items"]),
    )


def save_stat_state(path: Path, state: StatApplicationState) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {"version": STATE_VERSION, **asdict(state)}
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def count_stat_items(item_ids: list[int]) -> tuple[int, int]:
    return item_ids.count(MAX_HP_ITEM_ID), item_ids.count(MAX_MP_ITEM_ID)
