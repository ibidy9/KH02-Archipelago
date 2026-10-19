"""Pointer-free effects-off fresh setup, sealed into the prelaunch intent nonce.

Caller must supply current authenticated history and recheck it before publishing
the intent. Encoding is not AP authentication and performs no file operations.
"""
import hashlib
from pathlib import PureWindowsPath
import struct
from .storage_session_candidate import scope_binding

HEADER=struct.Struct('<4I2Q8I32s16s16s')
MAGIC=0x4b465350
MAX_RESUME_RECORDS=100
MAX_SIZE=128+4096+4096*32+1023+32+MAX_RESUME_RECORDS*(8+0x3b40+102456)

def encode_setup(*,key,history,supported,cookie,generation,journal_path):
    binding=bytes.fromhex(scope_binding(key));seed=key.seed_name.encode('utf-8')
    if type(cookie) is not int or not 0<cookie<2**64:raise ValueError('Invalid cookie')
    if type(generation) is not str or len(generation)!=32 or any(x not in '0123456789abcdef' for x in generation) or int(generation,16)==0:
        raise ValueError('Fresh random generation required')
    if len(history)>4096 or tuple(i.index for i in history)!=tuple(range(len(history))):
        raise ValueError('Complete indexed history required')
    if not 0<len(supported)<=4096 or len(set(supported))!=len(supported):raise ValueError('Invalid supported items')
    path=journal_path.encode('utf-8');p=PureWindowsPath(journal_path)
    if not p.is_absolute() or len(p.drive)!=2 or '..' in p.parts or '\0' in journal_path or not 0<len(path)<=1023:
        raise ValueError('Absolute local journal path required')
    body=seed+b''.join(struct.pack('<QQQ',i.index,i.item_id,i.player) for i in history)
    body+=b''.join(struct.pack('<Q',i) for i in supported)+path+generation.encode('ascii')
    return HEADER.pack(MAGIC,1,128+len(body),1,cookie,1,key.team,key.slot,len(seed),len(history),len(supported),len(path),32,0,binding,bytes.fromhex(generation),bytes(16))+body

def setup_nonce(payload):
    """Use this digest as startup request.nonce; never reseal after launch."""
    return hashlib.sha256(payload).digest()
