"""Mickey behavior choices."""
from dataclasses import dataclass
from Options import Choice
from .phantom_options_v100_candidate import PhantomOptions

class MickeyMode(Choice):
    """Keep Mickey's normal behavior, or unlock him as an item for Depths only or all worlds."""
    display_name = 'Mickey'
    option_vanilla = 0
    option_randomized_depths = 1
    option_randomized_all_worlds = 2
    default = 0

@dataclass
class MickeyOptions(PhantomOptions):
    mickey: MickeyMode
