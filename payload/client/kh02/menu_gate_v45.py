"""AP-derived menu locking; no development write commands."""
import ctypes as c
from .magic_predicate_v41 import MagicPredicateBlock
from .magic_ap_v42 import WindowsMagicAPTransport

class MenuObservation(c.Structure):
    _fields_=[('raw',c.c_uint64),('reasons',c.c_uint64*9),
              ('accepted',(c.c_uint64*7)*7),('native_true',(c.c_uint64*7)*7),('blocked',(c.c_uint64*7)*7)] + [
        (n,c.c_uint64) for n in ('epoch','last_hud','last_vtable','last_actor',
        'last_caller','last_command','last_row','last_result','last_row_bytes','last_row_tail')]

class MenuObservationBlock(MagicPredicateBlock):
    _fields_=[('menu_observation',MenuObservation)]

class WindowsMenuObservationTransport(WindowsMagicAPTransport):
    block_type=MenuObservationBlock
    def menu_observation_status(self):
        p=self.block.menu_observation
        return {n:([list(row) for row in getattr(p,n)] if n in ('accepted','native_true','blocked')
                   else list(getattr(p,n)) if n=='reasons' else int(getattr(p,n)))
                for n,_ in p._fields_}
