"""Wayfinder ownership is server history, not a game-save effect.

Only the three immutable Wayfinder identities are omitted. Every remaining
receipt retains its original index and the proven one-effect transaction.
"""
from dataclasses import replace
from .goal_items_v79_candidate import WAYFINDER_IDS
from .goal_policy_v90_candidate import build_goal_policy
from .production_policy import ReceiptPlan
from .production_schema import ItemCategory
from .receipt_set_candidate import encode

HISTORY_ONLY=frozenset(WAYFINDER_IDS.values())

def build_policy(history):
    # Native policy ABI stays at proven schema4: no Wayfinder command6 effect.
    return replace(build_goal_policy(history),schema_version=4)

def plan_receipts(policy,membership):
    encode(membership)
    received=policy.items;applied=membership.items
    if any(i.index>=len(received) or received[i.index]!=i or i.item_id in HISTORY_ONLY for i in applied):
        raise ValueError('Saved membership is not an effect-only authenticated subset')
    indices={i.index for i in applied}
    if len(indices)!=len(applied):raise ValueError('Duplicate saved receipt')
    for item,d in zip(received,policy.descriptors):
        if item.index in indices or item.item_id in HISTORY_ONLY:continue
        native=d.category in (ItemCategory.NATIVE_CONSUMABLE,ItemCategory.NATIVE_WARDROBE)
        proposed=applied if native else tuple(sorted(applied+(item,),key=lambda i:i.index))
        return ReceiptPlan(policy,applied,() if native else (item,),item if native else None,proposed)
    return ReceiptPlan(policy,applied,(),None,applied)
