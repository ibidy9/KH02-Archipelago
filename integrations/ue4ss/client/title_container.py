"""Recognize the native title-only container left by an interrupted AP New Game."""

import hashlib
import json
import struct

from tools.save_filename_support import identity
from .steam_profiles_v187_candidate import plain_directory
from .storage_session_candidate import STEAM_BUILD

# Digest of the entire verified decoded SAVE image, including all 100 records
# and container metadata. Independent of the encrypted PNG's Steam account key.
TITLE_ONLY_SHA256 = "d4e726a9815e39cea7d8c7794388b7e1ff83db097a03492b0784c1d3d4473051"


def is_owned_title_container(raw, profile, metadata, associations):
    if associations or hashlib.sha256(raw).hexdigest() != TITLE_ONLY_SHA256:
        return False
    root = profile / "saves"
    claims = profile / "claims"
    plain_directory(root)
    plain_directory(claims)
    plain_directory(profile / "sessions")
    volume, index = identity(root)
    for session in (profile / "sessions").iterdir():
        plain_directory(session)
        record = session / "launch.json"
        ready = session / "prepared-ready.bin"
        if record.is_symlink() or ready.is_symlink() or not record.is_file() or not ready.is_file():
            continue
        launch = json.loads(record.read_text(encoding="utf-8"))
        if (
            launch.get("mode") != "fresh"
            or launch.get("container_state", "missing") != "missing"
            or launch.get("binding") != metadata["binding"]
            or launch.get("profile_generation") != metadata["generation"]
            or launch.get("profile") != str(profile)
        ):
            continue
        data = ready.read_bytes()
        if (
            len(data) != 1104
            or data[:16] != struct.pack("<8sII", b"KH02AP01", 1, 1104)
            or data[32:64].hex() != STEAM_BUILD
            or data[64:96].hex() != metadata["binding"]
            or struct.unpack_from("<IIQ", data, 128) != (volume, 0, index)
        ):
            continue
        claim = claims / (data[96:128].hex() + ".claim")
        if not claim.is_symlink() and claim.is_file() and claim.stat().st_size == 8:
            prepared, expires = struct.unpack_from("<QQ", data, 16)
            if prepared < int.from_bytes(claim.read_bytes(), "little") < expires:
                return True
    return False
