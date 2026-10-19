"""Explicit fresh-only setup with no imported record or applied receipts."""
from dataclasses import replace
import hashlib,struct
from .fresh_transport_host_candidate import prepare_fresh

def prepare_replay(session,storage,*,cookie,grant_nonce,supported,journal_path):
    if type(grant_nonce) is not int or not 0<grant_nonce<2**64:
        raise ValueError('Nonzero fresh replay authorization required')
    prepared=prepare_fresh(session,storage,cookie=cookie,supported=supported,journal_path=journal_path)
    payload=bytearray(prepared.payload)
    struct.pack_into('<I',payload,12,5)
    struct.pack_into('<Q',payload,120,grant_nonce)
    payload=bytes(payload)
    return replace(prepared,payload=payload,nonce=hashlib.sha256(payload).digest())
