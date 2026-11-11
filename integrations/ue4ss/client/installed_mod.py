"""Read-only preflight for the explicitly installed UE4SS/startup distribution.

The client neither repairs nor removes these files, including cooked paks.
Only readiness records belong to the temporary session transaction.
"""

import hashlib
import json
from pathlib import Path, PurePosixPath


def verify_installed_mod(package_root, binaries):
    manifest = json.loads((Path(package_root) / "installed-mod.json").read_text(encoding="utf-8"))
    if manifest.get("version") != 2 or not isinstance(manifest.get("files"), list):
        raise ValueError("Unsupported installed-mod manifest")
    binaries = Path(binaries).absolute()
    roots = {"binaries": binaries, "paks": binaries.parent.parent / "Content/Paks/~mods"}
    seen = set()
    for row in manifest["files"]:
        name, expected = row["path"], row["sha256"]
        root = row.get("root")
        if root not in roots:
            raise ValueError("Unknown installed-mod root")
        if not isinstance(name, str) or "\\" in name or ":" in name:
            raise ValueError("Invalid installed-mod relative path")
        relative = PurePosixPath(name)
        if (
            relative.is_absolute()
            or ".." in relative.parts
            or not relative.parts
            or relative.as_posix() != name
        ):
            raise ValueError("Installed-mod path escapes the selected game")
        identity = root + ":" + name.casefold()
        if identity in seen:
            raise ValueError("Duplicate installed-mod path")
        seen.add(identity)
        if (
            not isinstance(expected, str)
            or len(expected) != 64
            or any(c not in "0123456789abcdef" for c in expected)
        ):
            raise ValueError("Invalid installed-mod hash")
        target = roots[root].joinpath(*relative.parts)
        for path in (target, *target.parents):
            if path.is_symlink() or path.is_junction():
                raise ValueError("Linked mod installation preserved: " + name)
        if not target.is_file():
            raise ValueError(
                "KH 0.2 mod file is missing: "
                + name
                + ". Install the matching mod before launching the game."
            )
        if hashlib.sha256(target.read_bytes()).hexdigest() != expected:
            raise ValueError(
                "KH 0.2 mod file differs: "
                + name
                + ". Reinstall the matching mod while the game is closed; this client will not overwrite it."
            )
    if (
        not {
            "binaries:dinput8.dll",
            "binaries:ue4ss.dll",
            "binaries:dwmapi.dll",
            "binaries:mods/kh02receipthost/dlls/main.dll",
            "paks:kh02_ap_restartprototype_p.pak",
            "paks:tresgame-windowsnoeditor_99_p.pak",
        }
        <= seen
    ):
        raise ValueError("Installed-mod manifest is incomplete")
    return tuple(sorted(seen))
