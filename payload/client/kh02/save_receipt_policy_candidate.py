"""Offline specification of save-relative consumable replay, not a live adapter.

The adapter must prove the loaded checkpoint and drain the previous owner before
constructing a LoadedBranch. Inventory quantity is deliberately not an input.
Global audit records are never erased when a previous save is loaded.
"""
from dataclasses import dataclass

from .received import DeliveryKey, ReceivedItem


@dataclass(frozen=True)
class SaveCheckpoint:
    key: DeliveryKey
    snapshot_id: str  # Exact validated save identity, NOT slot number or timestamp.
    included: tuple[ReceivedItem, ...]  # Applied before snapshot, even if consumed.


@dataclass(frozen=True)
class LoadedBranch:
    checkpoint: SaveCheckpoint
    load_token: str  # Verified process + native load identity; never reused.
    applied_since_load: tuple[ReceivedItem, ...] = ()
    uncertain_since_load: bool = False


@dataclass(frozen=True)
class SaveReplayPlan:
    action: str
    reason: str
    pending: tuple[ReceivedItem, ...] = ()


def plan_save_replay(key: DeliveryKey, history: tuple[ReceivedItem, ...],
                     supported_ids: frozenset[int], branch: LoadedBranch | None,
                     current_load_token: str) -> SaveReplayPlan:
    """Plan pending consumables in the currently loaded branch only.

    Checkpoints are trusted evidence supplied by a future validated save adapter.
    This function cannot establish their authenticity or the loaded save itself.
    A newly proven reload creates a NEW branch, retaining old attempts in audit
    storage. Reconnection alone cannot clear uncertainty or create a new branch.
    """
    if branch is None or not current_load_token or branch.load_token != current_load_token:
        return SaveReplayPlan("pause", "Loaded save identity is not established")
    checkpoint = branch.checkpoint
    if checkpoint.key != key or not checkpoint.snapshot_id:
        return SaveReplayPlan("pause", "Checkpoint does not identify this AP save")
    if any(item.index != index for index, item in enumerate(history)):
        return SaveReplayPlan("pause", "Complete original AP history is required")
    included = {}
    for item in checkpoint.included + branch.applied_since_load:
        if (item.index in included or item.index < 0 or item.index >= len(history)
                or history[item.index] != item or item.item_id not in supported_ids):
            return SaveReplayPlan("pause", "Save receipt evidence conflicts with AP history")
        included[item.index] = item
    if branch.uncertain_since_load:
        return SaveReplayPlan("pause", "Current branch has an unresolved native effect")
    pending = tuple(item for item in history
                    if item.item_id in supported_ids and item.index not in included)
    return SaveReplayPlan("replay" if pending else "complete",
                          "Deliver only receipts absent from this save branch", pending)


def checkpoint_for_snapshot(branch: LoadedBranch, snapshot_id: str) -> SaveCheckpoint:
    """Construct candidate association only; DOES NOT establish a saved checkpoint.

    Caller must freeze effects, bind the exact serialized snapshot and durably
    associate it before save publication. A failed save cannot become the loaded
    checkpoint merely because this candidate object exists.
    """
    if branch.uncertain_since_load or not snapshot_id or not branch.load_token:
        raise ValueError("Cannot checkpoint unresolved or unidentified game state")
    receipts = branch.checkpoint.included + branch.applied_since_load
    if len({item.index for item in receipts}) != len(receipts):
        raise ValueError("Duplicate receipt in save association")
    return SaveCheckpoint(branch.checkpoint.key, snapshot_id,
                          tuple(sorted(receipts, key=lambda item: item.index)))
