"""Pure goal-access projection from a current authenticated AP history.

This does not send commands or mutate game state. Native admission is still
required before publication. Unknown non-goal items cannot contribute ownership.
"""
from dataclasses import dataclass
import json
from pathlib import Path
from .ap_history_candidate import HistorySnapshot,NetworkReceipt
from .goal_items_v79_candidate import GOAL_ITEM_IDS,ITEM_SCHEMA_VERSION
from .goal_access_candidate import Goal,TideRequirement,WORLDS
from .goal_access_v2_candidate import access
from .world_access import WORLD_ACCESS_ITEM_IDS


@dataclass(frozen=True)
class GoalConfiguration:
    item_schema: int
    selected_goal: Goal
    requirement: TideRequirement
    starting_world: str

    def validate(self):
        if (type(self.item_schema) is not int or self.item_schema!=ITEM_SCHEMA_VERSION
                or not isinstance(self.selected_goal,Goal)
                or not isinstance(self.requirement,TideRequirement)
                or self.starting_world not in WORLDS):
            raise ValueError('Explicit schema5 goal configuration required')


def project(snapshot,*,expected_key,configuration,is_current):
    configuration.validate()
    if (not isinstance(snapshot,HistorySnapshot) or snapshot.key!=expected_key
            or not is_current(snapshot) or len(snapshot.items)>4096):
        raise ValueError('Current authenticated room history required')
    catalog=json.loads((Path(__file__).resolve().parents[2]/'data/wardrobe-goal-identities-v78.json').read_text())
    names={v:k for k,v in GOAL_ITEM_IDS.items()}
    names.update({v:k+' Access' for k,v in WORLD_ACCESS_ITEM_IDS.items()})
    for row in catalog['rows']:
        if row['item_id'] in names:raise ValueError('Goal catalogue ID collision')
        names[row['item_id']]=row['name']
    owned=set()
    for receipt in snapshot.items:
        if (not isinstance(receipt,NetworkReceipt) or type(receipt.item) is not int
                or not 0<=receipt.item<2**64):
            raise ValueError('Malformed received-item identity')
        if receipt.item in names:owned.add(names[receipt.item])
    result=access(owned,starting_world=configuration.starting_world,
        selected_goal=configuration.selected_goal,requirement=configuration.requirement)
    if not is_current(snapshot):raise ValueError('History changed during goal projection')
    return result
