"""Versioned policy framing and receipt planning for the shared owner.

This is the host half of the compatibility ABI.  It never calls the game.  A
native implementation must validate the same frame and commit the projection
and membership atomically before this can be enabled by ProductionCoordinator.
"""
from __future__ import annotations

from dataclasses import dataclass
import struct
import hashlib

from .production_schema import ItemCategory, PolicySnapshot
from .received import DeliveryKey, ReceivedItem
from .receipt_set_candidate import ReceiptSet


POLICY_MAGIC = 0x4B485050  # KHPP
POLICY_VERSION = 1
POLICY_HEADER = struct.Struct("<IIIIQQQII32sIIIIII")
MAX_POLICY_REVISION = 2**64 - 1
SCOPE_TAG = b"KH02POLICY\x01\x00"


@dataclass(frozen=True)
class PolicyBinding:
    owner_cookie: int
    process_cookie: int
    authority: int
    revision: int
    scope_binding: bytes


@dataclass(frozen=True)
class ReceiptPlan:
    projection: PolicySnapshot
    already_applied: tuple[ReceivedItem, ...]
    policy_receipts: tuple[ReceivedItem, ...]
    native_item: ReceivedItem | None
    proposed_membership: tuple[ReceivedItem, ...]


def native_scope_binding(key: DeliveryKey) -> bytes:
    seed=key.seed_name.encode('utf-8')
    if (type(key.team) is not int or not 0<=key.team<2**32 or
        type(key.slot) is not int or not 0<key.slot<2**32 or
        not seed or len(seed)>4096):
        raise ValueError("Invalid native policy scope")
    return hashlib.sha256(SCOPE_TAG+struct.pack('<III',key.team,key.slot,len(seed))+seed).digest()


def encode_policy(binding: PolicyBinding, policy: PolicySnapshot) -> bytes:
    if (type(binding.owner_cookie) is not int or not 0 < binding.owner_cookie < 2**64
            or type(binding.process_cookie) is not int or not 0 < binding.process_cookie < 2**64
            or type(binding.authority) is not int or not 0 < binding.authority < 2**63
            or type(binding.revision) is not int or not 0 < binding.revision <= MAX_POLICY_REVISION
            or type(binding.scope_binding) is not bytes or len(binding.scope_binding) != 32):
        raise ValueError("Invalid policy binding")
    size = POLICY_HEADER.size
    return POLICY_HEADER.pack(
        POLICY_MAGIC, POLICY_VERSION, size, policy.schema_version,
        binding.owner_cookie, binding.process_cookie, binding.authority,
        binding.revision & 0xFFFFFFFF, binding.revision >> 32,
        binding.scope_binding, policy.hp_items, policy.mp_items,
        policy.ability_mask, policy.spell_mask, policy.world_mask, 0,
    )


def plan_receipts(policy: PolicySnapshot, membership: ReceiptSet) -> ReceiptPlan:
    """Plan one atomic forward step without filtering/reindexing AP history.

    Exactly one policy-only receipt is included. A following native
    award is identified but deliberately excluded: the proven command-5
    consumable/wardrobe transaction applies and commits that item separately.
    """
    received = policy.items
    applied = membership.items
    if len(applied) > len(received) or received[:len(applied)] != applied:
        raise ValueError("Native membership is not an authenticated history prefix")
    cursor = len(applied)
    logical: list[ReceivedItem] = []
    native = None
    native_categories = {ItemCategory.NATIVE_CONSUMABLE, ItemCategory.NATIVE_WARDROBE}
    while cursor < len(received):
        descriptor = policy.descriptors[cursor]
        item = received[cursor]
        if descriptor.category in native_categories:
            native = item
            break
        logical.append(item)
        cursor += 1
        break
    proposed = received[:cursor]
    return ReceiptPlan(policy, applied, tuple(logical), native, proposed)


def proposed_receipt_set(prior: ReceiptSet, plan: ReceiptPlan) -> ReceiptSet:
    if prior.items != plan.already_applied:
        raise ValueError("Receipt plan no longer matches native membership")
    return ReceiptSet(prior.key, prior.owner_cookie, prior.load_id,
                      prior.revision + 1, plan.proposed_membership)
