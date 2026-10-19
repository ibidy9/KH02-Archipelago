"""Offline candidate bridge from retained native witness to durable goal proof.

No process access or live watcher. Binding must be captured by the authenticated
shared-owner transport at admission, not reconstructed from this publication.
Keeping that binding permits journaling an ending after its owner has retired.
"""
from dataclasses import dataclass
import struct

from .goal_access_candidate import Goal
from .goal_completion_v2_candidate import GoalAdmission

WIRE = struct.Struct('<7Q3I4x')


@dataclass(frozen=True)
class WitnessBinding:
    admission: GoalAdmission
    cookie: int
    load_id: int
    selected_goal: Goal

    def validate(self):
        self.admission.validate()
        if (type(self.cookie) is not int or not 0 < self.cookie < 2**64
                or type(self.load_id) is not int or not 0 < self.load_id < 2**64
                or not isinstance(self.selected_goal, Goal)):
            raise ValueError('Authenticated receipt binding required')


def journal_witness(raw, binding, store):
    """Commit only an exact bound completed witness; return False if incomplete.

    Caller must obtain a stable publication from the bound process. This pure
    decoder does not authenticate a mapping or establish process identity.
    No acknowledgement or native state release occurs before durable commit.
    """
    binding.validate()
    if len(raw) != WIRE.size:
        raise ValueError('Unexpected witness size')
    cookie, epoch, load_id, owner, sample, phantom, tide, armed, completed, goal = WIRE.unpack(raw)
    expected_goal = 1 if binding.selected_goal == Goal.BOSS_RUSH else 2
    if ((cookie, epoch, load_id, owner, goal) !=
            (binding.cookie, binding.admission.epoch, binding.load_id,
             binding.admission.owner, expected_goal)):
        raise ValueError('Witness does not match original admitted receipt')
    if store.scope != dict(key=dict(seed_name=binding.admission.key.seed_name,
                                    team=binding.admission.key.team,
                                    slot=binding.admission.key.slot),
                           goal=binding.selected_goal.value):
        raise ValueError('Journal scope differs from admitted goal')
    if not sample or armed & ~expected_goal or completed not in (0, expected_goal):
        raise ValueError('Invalid native witness state')
    event = phantom if goal == 1 else tide
    other = tide if goal == 1 else phantom
    if other or (not completed and event) or (completed and not 0 < event <= sample):
        raise ValueError('Invalid native witness event')
    if not completed:
        return False
    store.commit(binding.admission)
    return True
