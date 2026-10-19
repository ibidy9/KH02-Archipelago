"""Starting abilities use AP precollected items and the existing receipt path."""
from dataclasses import dataclass
from Options import Choice, Toggle
from .deathlink_options_v253 import DeathLinkOptions

class StartCartwheel(Toggle):
    """Start with Cartwheel."""
    display_name = 'Start with Cartwheel'

class StartBarrier(Toggle):
    """Start with Barrier, on the ground and in the air."""
    display_name = 'Start with Barrier'

class StartCounterBlast(Toggle):
    """Start with Counter Blast. Barrier is also needed to use it."""
    display_name = 'Start with Counter Blast'

class StartAerialRecovery(Toggle):
    """Start with Aerial Recovery."""
    display_name = 'Start with Aerial Recovery'

class StartDoubleflight(Toggle):
    """Start with Doubleflight."""
    display_name = 'Start with Doubleflight'

class StartAirSlide(Toggle):
    """Start with Air Slide."""
    display_name = 'Start with Air Slide'

class StartShotlock(Toggle):
    """Start with Shotlock."""
    display_name = 'Start with Shotlock'

class StartSpellweaver(Toggle):
    """Start with Spellweaver."""
    display_name = 'Start with Spellweaver'

class StartCure(Toggle):
    """Start with Curaga."""
    display_name = 'Start with Curaga'

class StartingFire(Choice):
    """Start without Fire magic, with Firaga, or with Firaga and Firaja."""
    display_name = 'Starting Fire Magic'
    option_none = 0
    option_firaga = 1
    option_firaga_and_firaja = 2
    default = 0

class StartingBlizzard(Choice):
    """Start without Blizzard magic, with Blizzaga, or with Blizzaga and Blizzaja."""
    display_name = 'Starting Blizzard Magic'
    option_none = 0
    option_blizzaga = 1
    option_blizzaga_and_blizzaja = 2
    default = 0

class StartingThunder(Choice):
    """Start without Thunder magic, with Thundaga, or with Thundaga and Thundaja."""
    display_name = 'Starting Thunder Magic'
    option_none = 0
    option_thundaga = 1
    option_thundaga_and_thundaja = 2
    default = 0

@dataclass
class StartingLoadoutOptions(DeathLinkOptions):
    start_cartwheel: StartCartwheel
    start_barrier: StartBarrier
    start_counter_blast: StartCounterBlast
    start_aerial_recovery: StartAerialRecovery
    start_doubleflight: StartDoubleflight
    start_air_slide: StartAirSlide
    start_shotlock: StartShotlock
    start_spellweaver: StartSpellweaver
    start_cure: StartCure
    starting_fire: StartingFire
    starting_blizzard: StartingBlizzard
    starting_thunder: StartingThunder

LOADOUT_ITEMS = {
    'start_cartwheel': 'Cartwheel',
    'start_barrier': 'Barrier',
    'start_counter_blast': 'Counter Blast',
    'start_aerial_recovery': 'Aerial Recovery',
    'start_doubleflight': 'Doubleflight',
    'start_air_slide': 'Air Slide',
    'start_shotlock': 'Shotlock',
    'start_spellweaver': 'Spellweaver',
    'start_cure': 'Cure Magic',
    'starting_fire': 'Progressive Fire',
    'starting_blizzard': 'Progressive Blizzard',
    'starting_thunder': 'Progressive Thunder',
}

def configure(options):
    # Copy first: option defaults and callers' mappings must never be mutated.
    inventory = dict(options.start_inventory.value)
    replacements = {}
    for key, name in LOADOUT_ITEMS.items():
        amount = getattr(options, key).value
        if amount:
            inventory[name] = max(inventory.get(name, 0), amount)
            replacements[name] = amount
    options.start_inventory.value = inventory
    return replacements
