"""Read-only style-command diagnostics alongside unchanged v45 magic gating."""
import ctypes as c
from .menu_gate_v45 import MenuObservationBlock,WindowsMenuObservationTransport
class SituationObservation(c.Structure):
    _fields_=[('raw',c.c_uint64),('reasons',c.c_uint64*8),
        ('accepted',(c.c_uint64*4)*7),('native_true',(c.c_uint64*4)*7)]+[
        (n,c.c_uint64) for n in ('epoch','hud','controller','controller_vtable','actor',
        'character_kind','command','caller','row','result','ability_primary',
        'ability_reference','context','world','inventory_owner')]
class SituationObservationBlock(MenuObservationBlock):
    _fields_=[('situation_observation',SituationObservation)]
class WindowsSituationObservationTransport(WindowsMenuObservationTransport):
    block_type=SituationObservationBlock
    def situation_observation_status(self):
        p=self.block.situation_observation
        return {n:([list(row) for row in getattr(p,n)] if n in ('accepted','native_true')
            else list(getattr(p,n)) if n=='reasons' else int(getattr(p,n))) for n,_ in p._fields_}
