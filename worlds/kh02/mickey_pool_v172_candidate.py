"""Mickey consumes one filler slot; existing mandatory rewards are preserved."""
from .pool_v96_candidate import build as previous_build, Pool
from .pool_v95_candidate import ITEM_IDS as PREVIOUS_IDS
ITEM_IDS = dict(PREVIOUS_IDS, Mickey=0x020233)
if 0x020233 in PREVIOUS_IDS.values():
    raise ValueError('Mickey identity collision')

def build(*, mickey, unfilled_locations, **kwargs):
    if type(mickey) is not str or mickey not in ('vanilla', 'randomized_depths', 'randomized_all_worlds'):
        raise ValueError('Unknown Mickey mode')
    randomized = mickey != 'vanilla'
    result=previous_build(unfilled_locations=unfilled_locations-int(randomized), **kwargs)
    return Pool(result.mandatory + (('Mickey',) if randomized else ()), result.filler)
