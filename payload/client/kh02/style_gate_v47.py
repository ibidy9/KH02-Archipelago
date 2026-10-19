"""Independent Spellweaver receipt and request-gate diagnostics."""
import ctypes as c
from .situation_observer_v46 import SituationObservationBlock,WindowsSituationObservationTransport
from . import progressive_magic_v43 as prior
from .production_schema import ItemDescriptor,ItemCategory
SPELLWEAVER_ID=0x020208
MAGIC_IDS=prior.MAGIC_IDS|{SPELLWEAVER_ID}
# Keep the established policy builder's validation; add only the new handler.
namespace=dict(vars(prior))
namespace['ACTION_BITS']=dict(prior.ACTION_BITS)|{SPELLWEAVER_ID:0,0x020205:0,0x020206:0,0x020207:0}
import types
build_magic_policy=types.FunctionType(prior.build_magic_policy.__code__,namespace)
namespace['build_magic_policy']=build_magic_policy
magic_mask=types.FunctionType(prior.magic_mask.__code__,namespace)
class StyleGate(c.Structure):
    _fields_=[('raw',c.c_uint64),('reasons',c.c_uint64*7),
        ('accepted',c.c_uint64*2),('blocked',c.c_uint64*2)]+[
        (n,c.c_uint64) for n in ('epoch','controller','actor','command','caller','owned')]
class StyleGateBlock(SituationObservationBlock):
    _fields_=[('style_gate',StyleGate)]
class WindowsStyleGateTransport(WindowsSituationObservationTransport):
    block_type=StyleGateBlock
    def style_gate_status(self):
        p=self.block.style_gate
        return {n:list(getattr(p,n)) if n in ('reasons','accepted','blocked') else int(getattr(p,n)) for n,_ in p._fields_}
