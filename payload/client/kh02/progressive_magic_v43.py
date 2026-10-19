"""Schema4 progressive ownership, derived afresh from indexed AP history.

New IDs avoid reinterpreting old independent-spell receipts. Duplicate delivery
of a history snapshot is not another tier; distinct receipt indices are.
"""
from collections import Counter
from types import MappingProxyType
from .default_action_items import ACTION_BITS
from .production_schema import ITEM_DESCRIPTORS,ItemDescriptor,ItemCategory,PolicySnapshot,MAX_HISTORY
from .received import ReceivedItem

PROGRESSIVE_ITEMS=MappingProxyType({
    'Progressive Fire':(0x02021d,1,2),
    'Progressive Blizzard':(0x02021e,4,8),
    'Progressive Thunder':(0x02021f,16,32),
})
CURE_ID=0x020204
MAGIC_IDS=frozenset((CURE_ID,)+(tuple(v[0] for v in PROGRESSIVE_ITEMS.values())))
MAGIC_POOL=('Progressive Fire',)*2+('Progressive Blizzard',)*2+('Progressive Thunder',)*2+('Cure Magic',)
ITEM_SCHEMA=4


def build_magic_policy(history):
    items=tuple(history);descriptors=[];worlds=0
    if len(items)>MAX_HISTORY:raise ValueError('History exceeds bound')
    for index,item in enumerate(items):
        if (not isinstance(item,ReceivedItem) or type(item.index) is not int or item.index!=index or
            type(item.item_id) is not int or type(item.player) is not int or not 0<=item.player<2**64):
            raise ValueError('Malformed indexed AP history')
        if item.item_id in MAGIC_IDS:d=ItemDescriptor(item.item_id,ItemCategory.SPELL_UNLOCK,0)
        elif item.item_id in ACTION_BITS:d=ItemDescriptor(item.item_id,ItemCategory.ABILITY_UNLOCK,0)
        else:
            d=ITEM_DESCRIPTORS.get(item.item_id)
            if d is None or d.category not in (ItemCategory.NATIVE_CONSUMABLE,ItemCategory.NATIVE_WARDROBE,ItemCategory.LOGICAL_ACCESS):
                raise ValueError('Item has no progressive shared-owner handler')
        descriptors.append(d)
        if d.category is ItemCategory.LOGICAL_ACCESS:worlds|=d.value
    return PolicySnapshot(ITEM_SCHEMA,items,tuple(descriptors),0,0,0,0,worlds)


def magic_mask(history):
    counts=Counter(item.item_id for item in build_magic_policy(history).items)
    result=64 if counts[CURE_ID] else 0
    for item,base,grand in PROGRESSIVE_ITEMS.values():
        if counts[item]:result|=base
        if counts[item]>=2:result|=grand
    return result
