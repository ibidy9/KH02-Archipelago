"""Versioned Mickey slot contract; schema2 remains unchanged."""
from .goal_slot_v94_candidate import make_slot_data as goal_slot
MODES = ('vanilla', 'randomized_depths', 'randomized_all_worlds')

def make_slot_data(*, mickey, **kwargs):
    if type(mickey) is not str or mickey not in MODES:
        raise ValueError('Unknown Mickey mode')
    result = goal_slot(**kwargs)
    result.update(schema_version=3, mickey=mickey)
    return result
