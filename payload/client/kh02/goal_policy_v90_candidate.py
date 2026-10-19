"""Schema5 logical receipt composition; no Zodiac/stat handler is invented."""
from dataclasses import replace
from .style_gate_v47 import build_magic_policy
from .goal_items_v79_candidate import WAYFINDER_IDS
from .production_schema import ItemDescriptor,ItemCategory,PolicySnapshot,MAX_HISTORY
from .received import ReceivedItem

def build_goal_policy(history):
    items=tuple(history);descriptors=[];worlds=0
    if len(items)>MAX_HISTORY:raise ValueError('History exceeds bound')
    for index,item in enumerate(items):
        if (not isinstance(item,ReceivedItem) or type(item.index) is not int or item.index!=index or
            type(item.item_id) is not int or not 0<=item.item_id<2**64 or
            type(item.player) is not int or not 0<=item.player<2**64):
            raise ValueError('Malformed indexed history')
        if item.item_id in WAYFINDER_IDS.values():
            descriptor=ItemDescriptor(item.item_id,ItemCategory.LOGICAL_ACCESS,0)
        else:
            # Validate through the proven catalog, without changing original
            # indices in the returned history or receipt membership.
            one=build_magic_policy((replace(item,index=0),))
            descriptor=one.descriptors[0];worlds|=one.world_mask
        descriptors.append(descriptor)
    return PolicySnapshot(5,items,tuple(descriptors),0,0,0,0,worlds)
