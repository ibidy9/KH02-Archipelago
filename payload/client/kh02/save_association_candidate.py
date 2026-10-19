"""Pure offline association rules; no persistence, process access or delivery.

DurableAssociation is trusted input from a future validated storage adapter.
Constructing it does not perform a flush or prove a save was published.
"""
from dataclasses import dataclass
import hashlib

from .received import DeliveryKey, ReceivedItem
from .save_receipt_policy_candidate import SaveCheckpoint, LoadedBranch, plan_save_replay


@dataclass(frozen=True)
class DurableAssociation:
    key: DeliveryKey
    record: bytes
    included: tuple[ReceivedItem, ...]
    format_version: str = 'kh02-steam-record-v1'


@dataclass(frozen=True)
class AssociationResolution:
    reason: str
    checkpoint: SaveCheckpoint | None = None


def resolve_disk_association(key: DeliveryKey, record: bytes,
                             associations: tuple[DurableAssociation, ...],
                             history: tuple[ReceivedItem, ...],
                             supported: frozenset[int]) -> AssociationResolution:
    """Resolve exact selected bytes, not slot numbers or inventory quantities.

    Caller must independently prove native load success, snapshot validity and
    storage durability before this result can seed a live branch. Disk records
    must never be substituted for runtime checkpoint capture/restore identity.
    """
    if type(record) is not bytes or len(record) != 0x3B40:
        return AssociationResolution('Invalid exact-record evidence')
    matches = [a for a in associations if a.key == key and a.record == record]
    if not matches:
        return AssociationResolution('Unknown snapshot; never assume empty history')
    membership = None
    for association in matches:
        if association.format_version != 'kh02-steam-record-v1':
            return AssociationResolution('Conflicting snapshot format')
        candidate = SaveCheckpoint(key, 'offline-validation', association.included)
        check = plan_save_replay(key, history, supported,
                                 LoadedBranch(candidate, 'offline'), 'offline')
        if check.action == 'pause':
            return AssociationResolution('Association conflicts with server history')
        normalized = tuple(sorted(association.included, key=lambda item: item.index))
        if membership is not None and normalized != membership:
            return AssociationResolution('Identical bytes have conflicting receipt membership')
        membership = normalized
    digest = hashlib.sha256(record).hexdigest()
    return AssociationResolution('Unique exact-snapshot membership',
                                 SaveCheckpoint(key, 'kh02-steam-record-v1:' + digest, membership))
