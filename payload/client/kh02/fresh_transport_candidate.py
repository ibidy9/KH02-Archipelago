"""Offline fresh transport ABI. Live enabling and packaging remain withheld."""
import ctypes as c
from .route_transport_candidate import RouteBlock
from .fresh_setup_candidate import MAX_SIZE

class FreshEvent(c.Structure):
    _fields_=[('kind',c.c_uint32),('thread',c.c_uint32),('epoch',c.c_uint64),
              ('result',c.c_uint64),('known',c.c_uint32),('complete',c.c_uint32)]

class FreshTransportBlock(RouteBlock):
    _fields_=[('setup_size',c.c_uint32),('reserved_setup',c.c_uint32),('setup',c.c_ubyte*MAX_SIZE),
              ('evidence_complete',c.c_uint32),('evidence_sequence',c.c_uint32),
              ('count',c.c_uint32),('overflow',c.c_uint32),('events',FreshEvent*32)]

class ReusableFreshTransportBlock(RouteBlock):
    _fields_=FreshTransportBlock._fields_[:-1]+[('events',FreshEvent*128)]
