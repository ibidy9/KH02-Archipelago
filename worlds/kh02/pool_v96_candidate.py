"""Complete count-balanced pool candidate; native/logic release gates remain.

Uses user-confirmed single chest rewards and expected Zodiac Phantom MP+10.
Does not register a World, install a runtime, or authorize native stat effects.
"""
from collections import Counter
from dataclasses import dataclass
from types import MappingProxyType

from .pool_v95_candidate import ITEM_IDS, plan, require_capacity

FILLER_WEIGHTS = MappingProxyType({
    'Potion': 35, 'Ether': 30, 'Hi-Potion': 20, 'Mega-Potion': 5,
    'Mega-Ether': 5, 'Elixir': 3, 'Megalixir': 2,
})
STAT_COPIES = MappingProxyType({'Max HP Increase': 6, 'Max MP Increase': 3})


@dataclass(frozen=True)
class Pool:
    mandatory: tuple[str, ...]
    filler: tuple[str, ...]

    @property
    def items(self):
        return self.mandatory + self.filler


def build(*, goal, requirement, starting_world, rng, unfilled_locations,
          randomized_world_access=True):
    """Use the world's seeded RNG and real unfilled item-bearing capacity.

    Never count events/locked placements as available slots. Overflow is refused
    before consuming RNG or altering any caller-owned pool or location objects.
    """
    value = plan(goal=goal, requirement=requirement, starting_world=starting_world,
                 randomized_world_access=randomized_world_access)
    counts = Counter(value['mandatory_item_counts'])
    counts.update(STAT_COPIES)
    mandatory = tuple(name for name in sorted(counts) for _ in range(counts[name]))
    if any(name not in ITEM_IDS for name in mandatory):
        raise ValueError('Mandatory item lacks an identity')
    spare = require_capacity(len(mandatory), unfilled_locations)
    filler = tuple(rng.choices(tuple(FILLER_WEIGHTS),
                               weights=tuple(FILLER_WEIGHTS.values()), k=spare))
    return Pool(mandatory, filler)
