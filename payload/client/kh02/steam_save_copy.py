"""Read-only copy/decode of the selected seed's own save container."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from .steam_profiles import plain_directory
from .steam_preparation import RAW_SIZE


def copy_and_decode(profile, session_directory, *, steam_id, require_closed,
                    decoder_script):
    if type(steam_id) is not str or not re.fullmatch(r'[0-9]{17}', steam_id):
        raise ValueError('A Steam account ID is required to verify this save')
    profile, session_directory = Path(profile).absolute(), Path(session_directory).absolute()
    plain_directory(profile)
    plain_directory(session_directory)
    if session_directory.parent != profile / 'sessions':
        raise ValueError('Save recovery must be inside this AP profile session')
    marker = profile / 'profile.json'
    if marker.is_symlink() or not marker.is_file():
        raise ValueError('AP profile ownership required; no vanilla save adoption')
    require_closed()
    save_root = profile / 'saves'
    plain_directory(save_root)
    candidates = [p for p in save_root.iterdir()
                  if p.name in ('KH0.2BbS_WW.png', 'KH0.2BbS_JP.png')]
    if not candidates:
        if any(save_root.iterdir()):
            raise ValueError('Unrecognized files in AP save folder; preserved for inspection')
        return None
    if len(candidates) != 1 or candidates[0].is_symlink() or not candidates[0].is_file():
        raise ValueError('Ambiguous or linked AP save container')
    source = candidates[0]
    data = source.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    destination = session_directory / source.name
    with destination.open('xb') as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())
    if destination.read_bytes() != data:
        raise ValueError('AP save recovery copy verification failed')
    require_closed()
    if source.read_bytes() != data:
        raise ValueError('AP save changed while copying; preparation refused')
    decoded = session_directory / 'decoded'
    subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File',
        str(Path(decoder_script).resolve(strict=True)), '-InputPath', str(destination),
        '-SteamId', steam_id, '-OutputDirectory', str(decoded)],
        check=True, capture_output=True, text=True, timeout=120, creationflags=0x08000000)
    report = json.loads((decoded / 'report.json').read_text(encoding='utf-8-sig'))
    raw = (decoded / 'decoded.raw').read_bytes()
    if (report.get('input_sha256', '').lower() != digest
            or report.get('raw_sha256', '').lower() != hashlib.sha256(raw).hexdigest()
            or report.get('raw_length') != RAW_SIZE or len(raw) != RAW_SIZE
            or report.get('png_crcs_verified') is not True
            or report.get('native_digest_verified') is not True):
        raise ValueError('Save decoder integrity proof failed')
    require_closed()
    if source.read_bytes() != data or destination.read_bytes() != data:
        raise ValueError('Save or recovery copy changed during verification')
    return raw, digest
