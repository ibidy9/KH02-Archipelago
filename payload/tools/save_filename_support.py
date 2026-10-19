"""Shared wire/evidence definitions for the bounded filename-only experiment."""
import ctypes as c
import hashlib
from pathlib import Path
import struct
import time
class Info(c.Structure):
    _fields_=[('attributes',c.c_uint32),('times',c.c_uint32*6),('volume',c.c_uint32),
              ('size_high',c.c_uint32),('size_low',c.c_uint32),('links',c.c_uint32),
              ('index_high',c.c_uint32),('index_low',c.c_uint32)]
class Event(c.Structure):
    _fields_=[('complete',c.c_int32),('thread',c.c_uint32),('caller',c.c_uint64),('name',c.c_wchar*12),('path',c.c_wchar*256)]
class CriticalMenuEvidence(c.Structure):
    _fields_=[(n,c.c_int32) for n in ('installed','calls','permitted','changed','last_before','last_after')]
class Evidence(c.Structure):
    _fields_=[(n,c.c_uint32) for n in ('magic','version','size','mode','startup_error','install_status','count','overflow','flight','fault')]+[('created',c.c_uint64),('binding',c.c_ubyte*32),('root',c.c_wchar*240),('critical_menu',CriticalMenuEvidence),('events',Event*32)]
def identity(path):
    k=c.WinDLL('kernel32',use_last_error=True)
    k.CreateFileW.argtypes=[c.c_wchar_p,c.c_uint32,c.c_uint32,c.c_void_p,c.c_uint32,c.c_uint32,c.c_void_p];k.CreateFileW.restype=c.c_void_p
    k.GetFileInformationByHandle.argtypes=[c.c_void_p,c.POINTER(Info)];k.CloseHandle.argtypes=[c.c_void_p]
    h=k.CreateFileW(str(path),0x80,7,None,3,0x2200000,None)
    if h==c.c_void_p(-1).value:raise OSError(c.get_last_error(),'Directory identity open failed')
    try:
        info=Info()
        if not k.GetFileInformationByHandle(h,c.byref(info)) or info.attributes&0x400 or not info.attributes&0x10:raise ValueError('Not a plain directory')
        return info.volume,(info.index_high<<32)|info.index_low
    finally:k.CloseHandle(h)
def filetime():return time.time_ns()//100+116444736000000000

def resume_wire(ready,container_sha256):
    if len(ready)!=1104 or ready[:8]!=b'KH02AP01':raise ValueError('Invalid startup wire')
    expected=bytes.fromhex(container_sha256)
    if len(expected)!=32:raise ValueError('Wrong container digest length')
    data=bytearray(144);data[:8]=b'KH02RS01'
    struct.pack_into('<II',data,8,1,144)
    data[16:80]=ready[64:128] # binding and one-use launch nonce
    data[80:96]=ready[128:144] # pinned root volume, reserved, file identity
    data[96:128]=expected
    data[128:144]=ready[16:32] # same preparation interval
    return bytes(data)
def wire(exe,save_root,claims,binding,nonce,*,prepared=None,lifetime_seconds=3600):
    if len(binding)!=32 or len(nonce)!=32:raise ValueError('Wrong binding/nonce length')
    now=filetime();prepared=now if prepared is None else prepared
    volume,index=identity(save_root);data=bytearray(1104);data[:8]=b'KH02AP01'
    struct.pack_into('<IIQQ',data,8,1,1104,prepared,now+lifetime_seconds*10000000)
    data[32:64]=hashlib.sha256(Path(exe).read_bytes()).digest();data[64:96]=binding;data[96:128]=nonce
    struct.pack_into('<IIQ',data,128,volume,0,index)
    for offset,path in ((144,save_root),(624,claims)):
        value=(Path(path).resolve().as_posix()+'/').encode('utf-16-le')
        if len(value)>=480:raise ValueError('Root too long')
        data[offset:offset+len(value)]=value
    return bytes(data)
