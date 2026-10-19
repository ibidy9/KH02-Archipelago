"""Additive identities for default actions; never reinterpret legacy game bits.

Ownership comes from complete authenticated AP history. These identities are
reserved for the v2 action integration; the old seed generator is unchanged.
"""
from types import MappingProxyType
from dataclasses import replace
from .production_schema import (ITEM_DESCRIPTORS, ItemDescriptor, ItemCategory,
                               PolicySnapshot, build_policy, MAX_HISTORY)
from .received import ReceivedItem

ACTION_ITEMS = MappingProxyType({
    'Cartwheel': (0x020216, 8),
    'Barrier': (0x020217, 4),
    'Counter Blast': (0x020218, 2),
    'Aerial Recovery': (0x020219, 1),
})
ACTION_BITS = MappingProxyType({item: bit for item, bit in ACTION_ITEMS.values()})
ACTION_ITEM_SCHEMA = 2


def build_action_policy(history):
    """Produce receipt descriptors without mixing logical actions into game masks."""
    items = tuple(history)
    if len(items) > MAX_HISTORY:
        raise ValueError('History exceeds bound')
    descriptors = []
    for index, item in enumerate(items):
        if (not isinstance(item, ReceivedItem) or type(item.index) is not int or
                item.index != index or type(item.item_id) is not int or
                type(item.player) is not int or not 0 <= item.player < 2**64):
            raise ValueError('Malformed authenticated history')
        if item.item_id in ACTION_BITS:
            # Actual action state is projected from the native authenticated
            # history, not from the old ability-field mask in this wire format.
            descriptors.append(ItemDescriptor(item.item_id, ItemCategory.ABILITY_UNLOCK, 0))
        else:
            descriptor = ITEM_DESCRIPTORS.get(item.item_id)
            if descriptor is None or descriptor.category not in (
                    ItemCategory.NATIVE_CONSUMABLE, ItemCategory.NATIVE_WARDROBE,
                    ItemCategory.LOGICAL_ACCESS):
                raise ValueError('Item has no v37 handler')
            descriptors.append(descriptor)
    world_mask = 0
    for descriptor in descriptors:
        if descriptor.category is ItemCategory.LOGICAL_ACCESS:
            world_mask |= descriptor.value
    return PolicySnapshot(ACTION_ITEM_SCHEMA, items, tuple(descriptors), 0, 0, 0, 0, world_mask)


def action_mask(history):
    policy = build_action_policy(history)
    result = 0
    for item in policy.items:
        result |= ACTION_BITS.get(item.item_id, 0)
    return result
