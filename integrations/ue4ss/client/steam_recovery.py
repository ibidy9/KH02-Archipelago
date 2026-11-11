"""Restore only verified AP startup additions from an interrupted launch."""

import hashlib
import json
import struct
from pathlib import Path
from tools.save_filename_support import filetime

from .steam_installation import Installation
from .steam_profiles import plain_directory
from .storage_session_candidate import STEAM_BUILD


def recover_interrupted(executable, profiles_root, *, require_closed):
    """Caller holds the runtime lease. Older preparations also need a claim or expiry."""
    require_closed()
    executable = Path(executable).absolute()
    plain_directory(executable.parent)
    names = ("kh02-ap-ready.bin", "kh02-ap-resume.bin")
    sizes = (1104, 144)
    outstanding = {}
    for name, size in zip(names, sizes):
        path = executable.parent / name
        if path.is_symlink() or (
            path.exists() and (not path.is_file() or path.stat().st_size != size)
        ):
            raise ValueError("Unrecognized startup file preserved: " + name)
        if path.exists():
            outstanding[name] = path.read_bytes()
    if not outstanding:
        return False
    ready, resume = (outstanding.get(name) for name in names)
    if ready is not None and ready[:16] != struct.pack("<8sII", b"KH02AP01", 1, sizes[0]):
        raise ValueError("Unrecognized readiness record preserved")
    if resume is not None and (
        resume[:8] not in (b"KH02RS01", b"KH02ES01")
        or resume[8:16] != struct.pack("<II", 1, sizes[1])
    ):
        raise ValueError("Unrecognized resume record preserved")
    identity = ready[64:128] if ready is not None else resume[16:80]
    if resume is not None and resume[16:80] != identity:
        raise ValueError("Startup files belong to different sessions; both preserved")
    binding = identity[:32].hex()
    root = Path(profiles_root).absolute()
    plain_directory(root)
    matches = []
    for profile in root.glob("*-" + binding[:16]):
        plain_directory(profile / "sessions")
        for session in (profile / "sessions").iterdir():
            plain_directory(session)
            archived = session / "prepared-ready.bin"
            if (
                archived.is_symlink()
                or not archived.is_file()
                or archived.stat().st_size != sizes[0]
            ):
                continue
            original = archived.read_bytes()
            if original[64:128] == identity and (ready is None or original == ready):
                matches.append((session, original))
    if len(matches) != 1:
        raise ValueError("Cannot identify a unique prior AP session for " + ", ".join(outstanding))
    session, original = matches[0]
    if (
        original[:16] != struct.pack("<8sII", b"KH02AP01", 1, sizes[0])
        or original[32:64].hex() != STEAM_BUILD
    ):
        raise ValueError("Prior startup record does not match this game build")
    launch_file = session / "launch.json"
    if launch_file.is_symlink():
        raise ValueError("Linked launch record preserved")
    launch = json.loads(launch_file.read_text(encoding="utf-8"))
    if launch["binding"] != binding:
        raise ValueError("Startup record and launch scope differ; session preserved")
    claims = session.parent.parent / "claims"
    plain_directory(claims)
    claim = claims / (identity[32:].hex() + ".claim")
    prepared, expires = struct.unpack_from("<QQ", original, 16)
    spent = (
        not claim.is_symlink()
        and claim.is_file()
        and claim.stat().st_size == 8
        and prepared < int.from_bytes(claim.read_bytes(), "little") < expires
    )
    if launch.get("exclusive_preparation") is not True and not spent and filetime() < expires:
        raise ValueError(
            "Previous AP launch is still reserved. Close the previous KH client normally or wait for its launch reservation to expire."
        )
    transaction = session / "installation"
    plain_directory(transaction)
    manifest = transaction / "installation.json"
    if manifest.is_symlink():
        raise ValueError("Linked recovery manifest preserved")
    rows = json.loads(manifest.read_text(encoding="utf-8"))["files"]
    recorded = {Path(row["path"]): row["sha256"] for row in rows}
    for name, contents in outstanding.items():
        if recorded.get(executable.parent / name) != hashlib.sha256(contents).hexdigest():
            raise ValueError("Startup file differs from the recorded session: " + name)
    recover(session, executable=executable, require_closed=require_closed)
    return True


def recover(directory, *, executable, require_closed):
    require_closed()
    directory, executable = Path(directory).absolute(), Path(executable).absolute()
    plain_directory(directory)
    plain_directory(executable.parent)
    if (
        executable.is_symlink()
        or hashlib.sha256(executable.read_bytes()).hexdigest() != STEAM_BUILD
    ):
        raise ValueError("Select the supported executable for this recovery")
    launch_file = directory / "launch.json"
    if launch_file.is_symlink():
        raise ValueError("Linked recovery metadata refused")
    launch = json.loads(launch_file.read_text(encoding="utf-8"))
    if Path(launch["executable"]).absolute() != executable:
        raise ValueError("Recovery belongs to a different game installation")
    profile = Path(launch["profile"]).absolute()
    plain_directory(profile)
    if directory.parent != profile / "sessions":
        raise ValueError("Recovery is outside its recorded AP profile")
    marker = profile / "profile.json"
    if marker.is_symlink():
        raise ValueError("Linked AP profile metadata refused")
    identity = json.loads(marker.read_text(encoding="utf-8"))
    if identity["binding"] != launch["binding"]:
        raise ValueError("Recovery AP identity differs")
    targets = [executable.parent / name for name in ("kh02-ap-ready.bin", "kh02-ap-resume.bin")]
    transaction = directory / "installation"
    plain_directory(transaction)
    if (transaction / "installation.json").is_symlink():
        raise ValueError("Linked installation manifest refused")
    Installation(transaction, require_closed=require_closed, allowed_targets=targets).restore()
    return directory
