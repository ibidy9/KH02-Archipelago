"""Candidate shared-memory ABI only; no live launcher or process reads."""
import ctypes as c
from .route_transport_candidate import RouteBlock
from .disk_observation_candidate import MAX_SETUP
from .save_evidence_candidate import Event,Publication
class CommandBlock(c.Structure):
    _fields_=[('magic',c.c_uint32),('version',c.c_uint32),('size',c.c_uint32),('command',c.c_uint32),
              ('cookie',c.c_uint64),('sequence',c.c_uint64),('epoch',c.c_uint64),('raw_size',c.c_uint32),
              ('prefix_size',c.c_uint32),('prefix',c.c_ubyte*64),('raw',c.c_ubyte*0x1725b0),
              ('response_sequence',c.c_uint64),('status',c.c_uint32),('reserved',c.c_uint32)]
class SaveTransportBlock(RouteBlock):
    _fields_=[('setup_size',c.c_uint32),('path_size',c.c_uint32),('setup',c.c_ubyte*MAX_SETUP),('path',c.c_ubyte*1024),
              ('request',CommandBlock),('count',c.c_int32),('overflow',c.c_int32),('publication_count',c.c_int32),
              ('reserved',c.c_int32),('events',Event*32),('publications',Publication*4)]
