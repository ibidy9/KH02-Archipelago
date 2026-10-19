"""Schema4 carries exact half-units, avoiding float wire ambiguity."""
from .mickey_slot_v172_candidate import make_slot_data as previous

def make_slot_data(*, xp_multiplier, **kwargs):
    if type(xp_multiplier) is not int or xp_multiplier not in (2, 3, 4, 6, 8):
        raise ValueError('Unsupported XP multiplier')
    result = previous(**kwargs)
    result.update(schema_version=4, xp_multiplier_halves=xp_multiplier)
    return result
