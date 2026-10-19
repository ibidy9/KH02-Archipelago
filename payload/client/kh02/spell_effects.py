from __future__ import annotations

FIRE_ITEM_ID = 0x020201
BLIZZARD_ITEM_ID = 0x020202
THUNDER_ITEM_ID = 0x020203
CURE_ITEM_ID = 0x020204

SPELL_ITEM_BITS = {
    FIRE_ITEM_ID: 0x01,
    BLIZZARD_ITEM_ID: 0x02,
    THUNDER_ITEM_ID: 0x04,
    CURE_ITEM_ID: 0x08,
}
SPELL_FAMILY_MASK = 0x0F


def received_spell_mask(item_ids) -> int:
    mask = 0
    for item_id in item_ids:
        mask |= SPELL_ITEM_BITS.get(item_id, 0)
    return mask


def validate_spell_mask(mask: int) -> None:
    if not 0 <= mask <= SPELL_FAMILY_MASK:
        raise ValueError(f"Unsupported spell-family mask: 0x{mask:X}")
