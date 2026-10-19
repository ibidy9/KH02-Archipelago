from dataclasses import dataclass

from Options import Choice, PerGameCommonOptions, Toggle


STARTING_WORLD_NAMES = (
    "Castle Town",
    "The World Within",
    "Forest of Thorns",
    "Depths of Darkness",
)


class RandomizedWorldAccess(Toggle):
    """Replace the vanilla linear route with independently unlocked worlds."""

    display_name = "Randomized World Access"
    default = 0


class StartingWorld(Choice):
    """World available without its corresponding access item."""

    display_name = "Starting World"
    option_castle_town = 0
    option_the_world_within = 1
    option_forest_of_thorns = 2
    default = 0


@dataclass
class KH02Options(PerGameCommonOptions):
    """Generation options for Kingdom Hearts 0.2."""

    randomized_world_access: RandomizedWorldAccess
    starting_world: StartingWorld
