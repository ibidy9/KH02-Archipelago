"""Automatic shared-meter access diagnostics; no arbitrary write command."""
import ctypes as c
from .style_gate_v47 import StyleGateBlock,WindowsStyleGateTransport
class MeterAccess(c.Structure):
    _fields_=[(n,c.c_uint64) for n in ('calls','applied','unchanged','refused','uncertain',
        'no_ownership','epoch','owner','magic','style','before','after','reference','status')]
class MeterAccessBlock(StyleGateBlock):
    _fields_=[('meter_access',MeterAccess)]
class WindowsMeterAccessTransport(WindowsStyleGateTransport):
    block_type=MeterAccessBlock
    def meter_access_status(self):
        p=self.block.meter_access
        return {n:int(getattr(p,n)) for n,_ in p._fields_}
