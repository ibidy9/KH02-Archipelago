"""Fixed-size magic diagnostics on the established AP owner; no mutation API."""
import ctypes as c
from .action_ownership_v37 import ActionGateBlock, WindowsActionOwnershipTransport

class MagicRow(c.Structure):
    _fields_=[(n,c.c_uint32) for n in ('command','flags','effect','voice')]+[(n,c.c_uint64) for n in ('projectile','trailing')]

class MagicObservation(c.Structure):
    _fields_=[(n,c.c_uint32) for n in ('status','count','capacity','reserved')]+[
        (n,c.c_uint64) for n in ('epoch','provider','equipment','equipment_vtable','magic','magic_vtable','records','replacement')]+[('rows',MagicRow*7)]

class MagicBlock(ActionGateBlock):
    _fields_=[('magic_observation',MagicObservation)]

class WindowsMagicObservationTransport(WindowsActionOwnershipTransport):
    block_type=MagicBlock

    def magic_observation(self):
        value=self.block.magic_observation
        return {**{n:int(getattr(value,n)) for n,_ in MagicObservation._fields_ if n!='rows'},
                'rows':[{n:int(getattr(row,n)) for n,_ in MagicRow._fields_} for row in value.rows]}
