"""Recoverable installation of exact AP additions; never alters save files."""
import hashlib
import json
from pathlib import Path


def digest(data):
    return hashlib.sha256(data).hexdigest()


class Installation:
    def __init__(self, directory, *, require_closed, allowed_targets):
        self.directory = Path(directory).absolute()
        self.require_closed = require_closed
        self.allowed_targets = frozenset(Path(p).absolute() for p in allowed_targets)
        names = {'dinput8.dll', 'kh02-ap-ready.bin', 'kh02-ap-resume.bin',
                 'TresGame-WindowsNoEditor_99_P.pak', 'KH02_AP_RestartPrototype_P.pak'}
        if not self.allowed_targets or any(p.name not in names for p in self.allowed_targets):
            raise ValueError('Only explicit AP startup/mod files may be installed or removed')
        self.manifest = self.directory / 'installation.json'

    def install(self, additions):
        """Only absent files are added. Conflicting installations are preserved."""
        self.require_closed()
        if self.manifest.exists():
            raise ValueError('Installation already recorded; recover it before another launch')
        additions = tuple((Path(path).absolute(), data) for path, data in additions)
        if len({str(p).casefold() for p, _ in additions}) != len(additions):
            raise ValueError('Duplicate installation target')
        for path, data in additions:
            if path not in self.allowed_targets:
                raise ValueError('Target is outside the selected AP installation')
            if type(data) is not bytes or not path.parent.is_dir():
                raise ValueError('Existing installation directory and immutable bytes required')
            if path.exists() or path.is_symlink():
                raise ValueError('Existing file preserved: ' + str(path))
        self.directory.mkdir(parents=True, exist_ok=True)
        rows = []
        for index, (path, data) in enumerate(additions):
            recovery = self.directory / f'addition-{index}.bin'
            with recovery.open('xb') as stream:
                stream.write(data)
            if digest(recovery.read_bytes()) != digest(data):
                raise ValueError('Recovery verification failed')
            rows.append(dict(path=str(path), recovery=recovery.name, sha256=digest(data)))
        # Persist recovery intent before the first change to the installation.
        with self.manifest.open('x', encoding='utf-8') as stream:
            json.dump(dict(version=1, files=rows), stream, indent=2)
        for path, data in additions:
            self.require_closed()
            with path.open('xb') as stream:
                stream.write(data)
            if path.read_bytes() != data:
                raise ValueError('Installed bytes differ; recovery retained')
        return rows

    def restore(self):
        """Remove only this transaction's verified additions after normal exit."""
        self.require_closed()
        value = json.loads(self.manifest.read_text(encoding='utf-8'))
        if value.get('version') != 1:
            raise ValueError('Unknown installation recovery format')
        rows = value['files']
        # Verify every existing target before removing any of them.
        for row in rows:
            path = Path(row['path'])
            if path not in self.allowed_targets:
                raise ValueError('Recovery target is outside the selected AP installation')
            recovery = self.directory / row['recovery']
            if recovery.parent != self.directory or digest(recovery.read_bytes()) != row['sha256']:
                raise ValueError('Recovery archive changed')
            if path.is_symlink() or (path.exists() and digest(path.read_bytes()) != row['sha256']):
                raise ValueError('Changed installed file preserved: ' + str(path))
        for row in rows:
            self.require_closed()
            path = Path(row['path'])
            if path.exists():
                if digest(path.read_bytes()) != row['sha256']:
                    raise ValueError('Installed file changed during recovery')
                path.unlink()
        return True
