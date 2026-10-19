"""One per-user Windows hook-module path, independent of seed/package/version.

Only a verified packaged DLL is staged, while KH is closed. An exclusive OS
file lease lasts through verified game exit and native owner cleanup. Old bytes
are preserved before upgrades. This bounds path registration; it does not claim
to repair an already exhausted Windows hook-module table.
"""
import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import secrets

from .steam_profiles import plain_directory


def digest(data):
    return hashlib.sha256(data).hexdigest()


def default_root():
    base=os.environ.get('LOCALAPPDATA')
    if not base:
        raise RuntimeError('LOCALAPPDATA is required for the stable KH runtime')
    return Path(base)/'KH02AP'/'runtime'


class RuntimeLease:
    def __init__(self, *, require_closed, root=None):
        self.root=Path(root) if root is not None else default_root()
        self.require_closed=require_closed
        self.handle=None
        self.kernel=c.WinDLL('kernel32',use_last_error=True)
        self.kernel.CreateFileW.argtypes=[c.c_wchar_p,c.c_uint32,c.c_uint32,c.c_void_p,c.c_uint32,c.c_uint32,c.c_void_p]
        self.kernel.CreateFileW.restype=c.c_void_p
        self.kernel.CloseHandle.argtypes=[c.c_void_p]
        self.kernel.CloseHandle.restype=c.c_int

    def __enter__(self):
        self.require_closed()
        plain_directory(self.root);self.root.mkdir(parents=True,exist_ok=True)
        lock=self.root/'owner.lock'
        if lock.is_symlink() or lock.is_dir():
            raise ValueError('Invalid runtime lease path')
        self.handle=self.kernel.CreateFileW(str(lock),0xC0000000,0,None,4,0x80,None)
        if self.handle==c.c_void_p(-1).value:
            self.handle=None
            raise RuntimeError('Another KH client owns the runtime, or its lease is inaccessible')
        return self

    def stage(self, source, expected_hash):
        if self.handle is None:
            raise RuntimeError('Runtime staging requires its exclusive lease')
        self.require_closed()
        source=Path(source)
        plain_directory(source.parent)
        if source.is_symlink():raise ValueError('Linked runtime source')
        data=source.read_bytes()
        if digest(data)!=expected_hash:
            raise ValueError('Packaged runtime hash differs')
        target=self.root/'Kh02FreshTransport.dll';marker=self.root/'runtime.json'
        for p in (target,marker):
            if p.is_symlink() or p.is_dir():raise ValueError('Invalid staged runtime file')
        if target.exists() or marker.exists():
            if not target.is_file() or not marker.is_file():
                raise ValueError('Incomplete runtime staging; files preserved for recovery')
            old=target.read_bytes();old_hash=digest(old)
            metadata=json.loads(marker.read_text())
            if metadata!={'version':1,'sha256':old_hash}:
                raise ValueError('Unknown or changed staged runtime; preserved')
            if old_hash==expected_hash:return target
            backup=self.root/'backups';plain_directory(backup);backup.mkdir(exist_ok=True)
            saved=backup/(old_hash+'.dll')
            if saved.is_symlink():raise ValueError('Linked runtime backup')
            if not saved.exists():
                with saved.open('xb') as f:f.write(old);f.flush();os.fsync(f.fileno())
            if saved.read_bytes()!=old:raise ValueError('Runtime recovery copy differs')
        self.require_closed()
        # Interrupted two-file publication fails closed on the next stage call.
        # Never load a mismatch and never silently adopt an orphaned DLL.
        for target_file,body in ((target,data),(marker,json.dumps(dict(version=1,sha256=expected_hash)).encode())):
            tmp=self.root/(target_file.name+'.'+secrets.token_hex(8)+'.tmp')
            with tmp.open('xb') as f:f.write(body);f.flush();os.fsync(f.fileno())
            os.replace(tmp,target_file)
        if digest(target.read_bytes())!=expected_hash:
            raise ValueError('Staged runtime verification failed')
        return target

    def __exit__(self,*exc):
        if self.handle is not None:
            if not self.kernel.CloseHandle(self.handle):raise c.WinError(c.get_last_error())
            self.handle=None
