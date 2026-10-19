"""Select an exact packaged runtime; incomplete compositions cannot be armed."""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path

from .steam_profiles import plain_directory


@dataclass(frozen=True)
class Runtime:
    mode: str
    starting_world: str
    startup: Path
    transport: Path
    prototype: Path
    menu: Path
    hashes: dict

    def read(self, name):
        path = getattr(self, name)
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != self.hashes[name]:
            raise ValueError('Packaged runtime changed: ' + name)
        return data


def select(root, *, mode, starting_world):
    root = Path(root).absolute()
    plain_directory(root)
    manifest = root / 'runtime-catalog.json'
    if manifest.is_symlink():
        raise ValueError('Runtime manifest cannot be a link')
    catalog = json.loads(manifest.read_text(encoding='utf-8'))
    if catalog.get('version') != 1:
        raise ValueError('Unsupported runtime catalog')
    rows = [r for r in catalog['runtimes']
            if r['mode'] == mode and r['starting_world'] == starting_world]
    if len(rows) != 1:
        raise ValueError('No unique runtime for the seed and fresh/resume mode')
    row = rows[0]
    if row.get('offline_validated') is not True:
        raise ValueError('Runtime composition is not yet ready for a live test')
    paths, hashes = {}, {}
    for name in ('startup', 'transport', 'prototype', 'menu'):
        entry = row[name]
        relative = Path(entry['path'])
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Runtime asset is outside the package')
        path = root / relative
        plain_directory(path.parent)
        if path.is_symlink() or not path.is_file():
            raise ValueError('Missing or linked packaged runtime')
        paths[name], hashes[name] = path, entry['sha256']
    result = Runtime(mode, starting_world, hashes=hashes, **paths)
    for name in paths:
        result.read(name)
    return result
