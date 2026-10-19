"""Add Zodiac native receipts without changing existing item meanings or ABI."""
from dataclasses import replace
from enum import Enum

from .wayfinder_receipt_v91_candidate import build_policy as previous_policy, HISTORY_ONLY
from .production_schema import ItemDescriptor, ItemCategory, PolicySnapshot, MAX_HISTORY
from .production_policy import ReceiptPlan
from .received import ReceivedItem
from .receipt_set_candidate import encode

ZODIAC_IDS = frozenset(range(0x020223, 0x02022f))


class ZodiacCategory(Enum):
    NATIVE_ZODIAC = 'native_zodiac'


def build_policy(history):
    items = tuple(history)
    if len(items) > MAX_HISTORY:
        raise ValueError('History exceeds bound')
    descriptors = []
    worlds = 0
    for index, item in enumerate(items):
        if (not isinstance(item, ReceivedItem) or type(item.index) is not int or item.index != index
                or type(item.item_id) is not int or not 0 <= item.item_id < 2**64
                or type(item.player) is not int or not 0 <= item.player < 2**64):
            raise ValueError('Malformed indexed history')
        if item.item_id in ZODIAC_IDS:
            descriptor = ItemDescriptor(item.item_id, ZodiacCategory.NATIVE_ZODIAC,
                                        0x900 + item.item_id - 0x020223)
        else:
            one = previous_policy((replace(item, index=0),))
            descriptor = one.descriptors[0]
            worlds |= one.world_mask
        descriptors.append(descriptor)
    return PolicySnapshot(4, items, tuple(descriptors), 0, 0, 0, 0, worlds)


def plan_receipts(policy, membership):
    if policy != build_policy(policy.items):
        raise ValueError('Policy does not match authenticated item identities')
    encode(membership)
    received, applied = policy.items, membership.items
    if any(i.index >= len(received) or received[i.index] != i or i.item_id in HISTORY_ONLY for i in applied):
        raise ValueError('Membership is not an effect-only authenticated subset')
    indices = {i.index for i in applied}
    if len(indices) != len(applied):
        raise ValueError('Duplicate saved receipt')
    for item, descriptor in zip(received, policy.descriptors):
        if item.index in indices or item.item_id in HISTORY_ONLY:
            continue
        native = descriptor.category in (ItemCategory.NATIVE_CONSUMABLE, ItemCategory.NATIVE_WARDROBE,
                                         ZodiacCategory.NATIVE_ZODIAC)
        proposed = applied if native else tuple(sorted(applied + (item,), key=lambda i: i.index))
        return ReceiptPlan(policy, applied, () if native else (item,), item if native else None, proposed)
    return ReceiptPlan(policy, applied, (), None, applied)
