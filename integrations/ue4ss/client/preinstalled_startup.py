"""Compatibility step: verify user-installed startup, own only session additions.

The transaction can own only readiness records. DLLs and paks are installed
and removed by the user; the client has no cleanup permission for those files.
"""

import json
from pathlib import Path
from .steam_installation import Installation, digest


class PreinstalledStartup(Installation):
    def __init__(self, directory, *, require_closed, allowed_targets):
        targets = tuple(Path(p).absolute() for p in allowed_targets)
        startup = [p for p in targets if p.name == "dinput8.dll"]
        if len(startup) != 1:
            raise ValueError("Exactly one selected startup DLL is required")
        self.startup = startup[0]
        temporary = [p for p in targets if p != self.startup]
        if any(
            p.parent != self.startup.parent
            or p.name not in ("kh02-ap-ready.bin", "kh02-ap-resume.bin")
            for p in temporary
        ):
            raise ValueError("Only session readiness files may be owned by the client")
        super().__init__(directory, require_closed=require_closed, allowed_targets=temporary)
        self.verification = self.directory / "preinstalled-startup.json"

    def _verify(self, expected):
        # Refuse redirected ancestors as well as a linked DLL itself.
        for p in (self.startup, *self.startup.parents):
            if p.is_symlink() or p.is_junction():
                raise ValueError("Linked startup installation preserved")
        if not self.startup.is_file() or digest(self.startup.read_bytes()) != expected:
            raise ValueError(
                "Installed KH 0.2 startup mod does not match this client; reinstall the matching mod while the game is closed"
            )

    def install(self, additions):
        self.require_closed()
        additions = tuple((Path(p).absolute(), data) for p, data in additions)
        startups = [data for p, data in additions if p == self.startup]
        if len(startups) != 1 or type(startups[0]) is not bytes:
            raise ValueError("One immutable expected startup binary required")
        expected = digest(startups[0])
        self._verify(expected)
        if self.manifest.exists() or self.verification.exists():
            raise ValueError("Recover the previous session before preparing another")
        self.directory.mkdir(parents=True, exist_ok=True)
        with self.verification.open("x", encoding="utf-8") as f:
            json.dump(dict(version=1, path=str(self.startup), sha256=expected), f)
        result = super().install([(p, data) for p, data in additions if p != self.startup])
        self._verify(expected)
        return result

    def restore(self):
        self.require_closed()
        value = json.loads(self.verification.read_text(encoding="utf-8"))
        if value.get("version") != 1 or Path(value["path"]) != self.startup:
            raise ValueError("Unexpected permanent startup verification record")
        self._verify(value["sha256"])
        # Parent manifest cannot contain the permanent DLL: it is not allowed.
        result = super().restore()
        self._verify(value["sha256"])
        return result
