"""Optional standard AP DeathLink; never alters the item or location pool."""
from dataclasses import dataclass
from Options import DeathLink
from .traps_v244 import TrapOptions

@dataclass
class DeathLinkOptions(TrapOptions):
    death_link: DeathLink
