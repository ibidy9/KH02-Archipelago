"""Seed XP options; vanilla thresholds and level-check rules are unchanged."""
from dataclasses import dataclass
from Options import Choice
from .mickey_options_v172_candidate import MickeyOptions

class XPMultiplier(Choice):
    """Choose how much experience enemies give."""
    display_name = 'XP Multiplier'
    option_1x = 2
    option_1_5x = 3
    option_2x = 4
    option_3x = 6
    option_4x = 8
    default = 4

@dataclass
class XPOptions(MickeyOptions):
    xp_multiplier: XPMultiplier
