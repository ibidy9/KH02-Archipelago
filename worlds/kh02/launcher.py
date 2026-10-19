import asyncio
import hashlib
from importlib.resources import files
import json
import os
from pathlib import Path, PurePosixPath
import sys
import zipfile
import io

def launch(*args):
    from worlds.LauncherComponents import launch as launch_component
    launch_component(main, name="KH02Client", args=args)

def unpack():
    resource = files(__package__)
    manifest = json.loads(resource.joinpath('client-manifest.json').read_text())
    data = resource.joinpath('client-payload.zip').read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != manifest['sha256']: raise ValueError('KH02 client payload changed')
    root = Path(os.environ['LOCALAPPDATA'])/'KH02AP/client-cache'/digest
    for p in (root, *root.parents):
        if p.is_symlink() or (hasattr(p, 'is_junction') and p.is_junction()):
            raise ValueError('Linked client cache refused')
    root.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        if set(archive.namelist()) != set(manifest['files']): raise ValueError('Unexpected payload files')
        for name, expected in manifest['files'].items():
            relative = PurePosixPath(name)
            if relative.is_absolute() or '..' in relative.parts or ':' in name or '\\' in name:
                raise ValueError('Unsafe client payload path')
            target = root.joinpath(*relative.parts)
            for p in (target, *target.parents):
                if p.is_symlink() or (hasattr(p, 'is_junction') and p.is_junction()):
                    raise ValueError('Linked payload destination refused')
            content = archive.read(name)
            if hashlib.sha256(content).hexdigest() != expected: raise ValueError('Payload hash mismatch')
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists():
                with target.open('xb') as stream: stream.write(content)
            if target.read_bytes() != content: raise ValueError('Modified client cache preserved; cannot launch')
    return root

def main(*args):
    if sys.platform != 'win32': raise RuntimeError('KH02 client currently supports Steam on Windows only')
    root = unpack()
    for name in ('client', 'tools'):
        if name in sys.modules: raise RuntimeError('Conflicting module already loaded: '+name)
    sys.path.insert(0, str(root))
    try:
        import sqlite3
    except ModuleNotFoundError:
        import struct
        if sys.version_info[:2] != (3, 13) or struct.calcsize('P') != 8:
            raise RuntimeError('This KH02 client needs Archipelago with 64-bit Python 3.13 for its bundled SQLite journal support')
        global _sqlite_dll_directory
        _sqlite_dll_directory = os.add_dll_directory(str(root/'sqlite-support'))
        sys.path.insert(0, str(root/'sqlite-support'))
        import sqlite3
    import Utils
    Utils.init_logging('KH02Client', exception_logger='Client')
    from types import SimpleNamespace
    if args:
        import argparse
        parser = argparse.ArgumentParser()
        parser.add_argument('--connection-check', action='store_true', required=True)
        parser.add_argument('--server', required=True)
        parser.add_argument('--slot', required=True)
        parsed = parser.parse_args(args)
        from client.kh02.steam_client import connection_check
        asyncio.run(connection_check(SimpleNamespace(server=parsed.server, slot=parsed.slot, password=None)))
        return
    from client.kh02.steam_client import run
    asyncio.run(run(SimpleNamespace(server=None, password=None, slot=None, gui=True,
        exe=None, steam_id=None, package=root,
        profiles=Path(os.environ['LOCALAPPDATA'])/'KH02AP/profiles', resume_slot=1)))
