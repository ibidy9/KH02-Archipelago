"""Versioned production inputs and immutable AP item policy.

This module is deliberately effect-free.  It validates authenticated history
and slot data before a storage preparation or native command can be published.
Only item identities whose existing behavior is understood are admitted.
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from types import MappingProxyType
from typing import Iterable, Mapping

from .ability_effects import ITEM_TO_ABILITY_BIT
from .received import ReceivedItem
from .spell_effects import SPELL_ITEM_BITS
from .stat_effects import MAX_HP_ITEM_ID, MAX_MP_ITEM_ID
from .world_access import WORLD_ACCESS_ITEM_IDS, WORLD_NAMES


SLOT_DATA_VERSION = 1
ITEM_SCHEMA_VERSION = 1
GAME = "Kingdom Hearts 0.2 Birth by Sleep"
MAX_HISTORY = 4096


class ItemCategory(Enum):
    NATIVE_CONSUMABLE = "native_consumable"
    NATIVE_WARDROBE = "native_wardrobe"
    ABSOLUTE_STAT = "absolute_stat"
    ABILITY_UNLOCK = "ability_unlock"
    SPELL_UNLOCK = "spell_unlock"
    LOGICAL_ACCESS = "logical_access"


@dataclass(frozen=True)
class ItemDescriptor:
    item_id: int
    category: ItemCategory
    value: int


CONSUMABLE_ITEM_IDS = frozenset({
    0x020209, 0x02020A, 0x02020D, 0x02020E,
    0x02020F, 0x020210, 0x020211,
})
WARDROBE_ITEM_IDS = frozenset(
    item_id for item_id in range(0x020300, 0x020334) if item_id != 0x02032D
)


def _descriptors() -> Mapping[int, ItemDescriptor]:
    values: dict[int, ItemDescriptor] = {}

    def add(item_id: int, category: ItemCategory, value: int) -> None:
        if item_id in values:
            raise RuntimeError(f"Duplicate production item id 0x{item_id:X}")
        values[item_id] = ItemDescriptor(item_id, category, value)

    for item_id in sorted(CONSUMABLE_ITEM_IDS):
        add(item_id, ItemCategory.NATIVE_CONSUMABLE, item_id)
    for item_id in sorted(WARDROBE_ITEM_IDS):
        add(item_id, ItemCategory.NATIVE_WARDROBE, item_id)
    add(MAX_HP_ITEM_ID, ItemCategory.ABSOLUTE_STAT, 5)
    add(MAX_MP_ITEM_ID, ItemCategory.ABSOLUTE_STAT, 10)
    for item_id, bit in ITEM_TO_ABILITY_BIT.items():
        add(item_id, ItemCategory.ABILITY_UNLOCK, bit)
    for item_id, bit in SPELL_ITEM_BITS.items():
        add(item_id, ItemCategory.SPELL_UNLOCK, bit)
    for bit, world_name in enumerate(WORLD_NAMES):
        add(WORLD_ACCESS_ITEM_IDS[world_name], ItemCategory.LOGICAL_ACCESS, 1 << bit)
    return MappingProxyType(values)


ITEM_DESCRIPTORS = _descriptors()


@dataclass(frozen=True)
class SlotData:
    randomized_world_access: bool
    starting_world: str
    goal: str
    minimal_clear_content: bool
    critical_available: bool


@dataclass(frozen=True)
class PolicySnapshot:
    schema_version: int
    items: tuple[ReceivedItem, ...]
    descriptors: tuple[ItemDescriptor, ...]
    hp_items: int
    mp_items: int
    ability_mask: int
    spell_mask: int
    world_mask: int


def validate_slot_data(value: object) -> SlotData:
    if type(value) is not dict:
        raise ValueError("Slot data must be an object")
    required = {
        "schema_version", "game", "item_schema_version",
        "randomized_world_access", "starting_world", "goal",
        "minimal_clear_content", "critical_available",
    }
    if set(value) != required:
        raise ValueError("Slot data fields do not match version 1")
    if (value["schema_version"] != SLOT_DATA_VERSION
            or value["item_schema_version"] != ITEM_SCHEMA_VERSION
            or value["game"] != GAME):
        raise ValueError("Unsupported slot-data scope or version")
    for field in ("randomized_world_access", "minimal_clear_content", "critical_available"):
        if type(value[field]) is not bool:
            raise ValueError(f"{field} must be boolean")
    if value["starting_world"] not in WORLD_NAMES:
        raise ValueError("Unsupported starting world")
    if value["goal"] not in ("demon_tide", "boss_rush"):
        raise ValueError("Unsupported goal")
    if value["minimal_clear_content"] is not True:
        raise ValueError("Production AP saves require the minimal clear-content profile")
    if value["critical_available"] is not True:
        raise ValueError("Production New Game must expose Critical difficulty")
    return SlotData(
        value["randomized_world_access"], value["starting_world"],
        value["goal"], value["minimal_clear_content"],
        value["critical_available"],
    )


def build_policy(items: Iterable[ReceivedItem]) -> PolicySnapshot:
    items = tuple(items)
    if len(items) > MAX_HISTORY:
        raise ValueError("Authenticated history exceeds production bound")
    descriptors: list[ItemDescriptor] = []
    for index, item in enumerate(items):
        if (not isinstance(item, ReceivedItem) or item.index != index
                or type(item.item_id) is not int or type(item.player) is not int
                or not 0 <= item.item_id < 2**64 or not 0 <= item.player < 2**64):
            raise ValueError("Malformed authenticated item history")
        descriptor = ITEM_DESCRIPTORS.get(item.item_id)
        if descriptor is None:
            raise ValueError(f"Unsupported AP item id 0x{item.item_id:X} at index {index}")
        descriptors.append(descriptor)
    return PolicySnapshot(
        ITEM_SCHEMA_VERSION, items, tuple(descriptors),
        sum(d.item_id == MAX_HP_ITEM_ID for d in descriptors),
        sum(d.item_id == MAX_MP_ITEM_ID for d in descriptors),
        _mask(descriptors, ItemCategory.ABILITY_UNLOCK),
        _mask(descriptors, ItemCategory.SPELL_UNLOCK),
        _mask(descriptors, ItemCategory.LOGICAL_ACCESS),
    )


def _mask(values: Iterable[ItemDescriptor], category: ItemCategory) -> int:
    result = 0
    for value in values:
        if value.category is category:
            result |= value.value
    return result
