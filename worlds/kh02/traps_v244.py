"""Optional traps replace only spare filler; mandatory items remain intact."""
from dataclasses import dataclass
from Options import Range
from .xp_options_v185_candidate import XPOptions
from .mickey_pool_v172_candidate import ITEM_IDS as PREVIOUS_IDS, build as previous_build
from .pool_v96_candidate import Pool

TRAP_IDS = {'MP Recharge Trap':0x020234,'Stun Trap':0x020235}
assert not set(TRAP_IDS.values()) & set(PREVIOUS_IDS.values())
ITEM_IDS = dict(PREVIOUS_IDS, **TRAP_IDS)

class TrapPercentage(Range):
    """Percentage of filler items replaced with traps. Zero disables traps."""
    display_name='Trap Percentage'
    range_start=0
    range_end=100
    default=0

class MPRechargeTrapWeight(Range):
    """How often MP Recharge is chosen among traps. Zero disables this trap."""
    display_name='MP Recharge Trap Weight'
    range_start=0
    range_end=100
    default=1

class StunTrapWeight(Range):
    """How often Stun is chosen among traps. Zero disables this trap."""
    display_name='Stun Trap Weight'
    range_start=0
    range_end=100
    default=1

@dataclass
class TrapOptions(XPOptions):
    trap_percentage: TrapPercentage
    mp_recharge_trap_weight: MPRechargeTrapWeight
    stun_trap_weight: StunTrapWeight

def build(*,trap_percentage,mp_weight,stun_weight,rng,**kwargs):
    if any(type(v) is not int or not 0<=v<=100 for v in (trap_percentage,mp_weight,stun_weight)):
        raise ValueError('Invalid trap settings')
    if trap_percentage and not (mp_weight or stun_weight):
        raise ValueError('Enabled traps require a nonzero trap weight')
    pool=previous_build(rng=rng,**kwargs)
    count=len(pool.filler)*trap_percentage//100
    if not count:
        return pool
    positions=rng.sample(range(len(pool.filler)),count)
    filler=list(pool.filler)
    for index,name in zip(positions,rng.choices(tuple(TRAP_IDS),weights=(mp_weight,stun_weight),k=count)):
        filler[index]=name
    return Pool(pool.mandatory,tuple(filler))
