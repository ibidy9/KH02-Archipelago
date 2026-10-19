"""Explicit initial autosave or manual-slot-1 resume; exact membership unchanged."""
import struct
from .resume_setup_candidate import encode_setup as original

def encode_setup(*,target=1,**kwargs):
    if type(target) is not int or target not in (0,1):
        raise ValueError('Initial resume target must be autosave 0 or manual slot 1')
    payload=bytearray(original(target=1,**kwargs))
    struct.pack_into('<I',payload,116,target)
    return bytes(payload)
