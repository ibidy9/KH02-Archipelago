"""Generation-only option; native goal ownership and slot schema are unchanged."""
from dataclasses import dataclass
from Options import Toggle
from .goal_options_v94_candidate import GoalOptions


class ZodiacPhantomImportantItems(Toggle):
    """Allow important items as rewards for defeating Zodiac Phantom.
    Turn off to avoid requiring this optional fight for anyone's items."""
    display_name = 'Zodiac Phantom Important Items'
    default = 0


@dataclass
class PhantomOptions(GoalOptions):
    zodiac_phantom_important_items: ZodiacPhantomImportantItems
