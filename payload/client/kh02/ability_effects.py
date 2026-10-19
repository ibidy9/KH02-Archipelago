from __future__ import annotations

from typing import Iterable


DOUBLEFLIGHT_ITEM_ID = 0x020205
AIR_SLIDE_ITEM_ID = 0x020206
SHOTLOCK_ITEM_ID = 0x020207
SPELLWEAVER_ITEM_ID = 0x020208

DOUBLEFLIGHT_BIT = 0x01
AIR_SLIDE_BIT = 0x02
SHOTLOCK_BIT = 0x10
SITUATION_COMMAND_BIT = 0x08
CONTROLLED_ABILITY_MASK = (
    DOUBLEFLIGHT_BIT | AIR_SLIDE_BIT | SITUATION_COMMAND_BIT | SHOTLOCK_BIT
)

ITEM_TO_ABILITY_BIT = {
    DOUBLEFLIGHT_ITEM_ID: DOUBLEFLIGHT_BIT,
    AIR_SLIDE_ITEM_ID: AIR_SLIDE_BIT,
    SHOTLOCK_ITEM_ID: SHOTLOCK_BIT,
    SPELLWEAVER_ITEM_ID: SITUATION_COMMAND_BIT,
}


def received_ability_mask(item_ids: Iterable[int]) -> int:
    mask = 0
    for item_id in item_ids:
        mask |= ITEM_TO_ABILITY_BIT.get(item_id, 0)
    return mask


def validate_target_mask(mask: int) -> None:
    if mask < 0 or mask & ~CONTROLLED_ABILITY_MASK:
        raise ValueError(f"Unsupported ability target mask: 0x{mask:X}")
