"""Generation options for the opt-in schema2 world integration candidate."""
from dataclasses import dataclass
from Options import Choice, Visibility
from .options import KH02Options, StartingWorld, RandomizedWorldAccess


class Goal(Choice):
    """Finish Demon Tide or the Zodiac Phantom at the end of the boss rush."""
    display_name = 'Goal'
    option_demon_tide = 0
    option_boss_rush = 1
    alias_zodiac_phantom = 1
    default = 0


class GoalRequirement(Choice):
    """Unlock the selected goal with Wayfinders, Zodiacs, or wardrobe sets."""
    display_name = 'Goal Requirement'
    option_wayfinders = 0
    option_zodiacs = 1
    option_wardrobe_sets = 2
    default = 0


class AllStartingWorlds(StartingWorld):
    """Choose where your adventure begins. Random Start includes Depths only if you start with Cartwheel or Barrier."""
    option_depths_of_darkness = 3
    option_random_start = 4

    @classmethod
    def from_text(cls, text):
        if text.lower() == 'random':
            return cls(cls.option_random_start)
        return super().from_text(text)


class IndependentWorldAccess(RandomizedWorldAccess):
    """Unlock worlds by finding their access items."""
    default = 1
    visibility = Visibility.none


@dataclass
class GoalOptions(KH02Options):
    randomized_world_access: IndependentWorldAccess
    starting_world: AllStartingWorlds
    goal: Goal
    goal_requirement: GoalRequirement
