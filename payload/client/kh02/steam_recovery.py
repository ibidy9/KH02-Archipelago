"""Restore only verified AP startup additions from an interrupted launch."""
import hashlib
import json
from pathlib import Path

from .steam_installation import Installation
from .steam_profiles import plain_directory
from .storage_session_candidate import STEAM_BUILD


def recover(directory, *, executable, require_closed):
    require_closed()
    directory, executable = Path(directory).absolute(), Path(executable).absolute()
    plain_directory(directory)
    plain_directory(executable.parent)
    if executable.is_symlink() or hashlib.sha256(executable.read_bytes()).hexdigest() != STEAM_BUILD:
        raise ValueError('Select the supported executable for this recovery')
    launch_file = directory / 'launch.json'
    if launch_file.is_symlink():
        raise ValueError('Linked recovery metadata refused')
    launch = json.loads(launch_file.read_text(encoding='utf-8'))
    if Path(launch['executable']).absolute() != executable:
        raise ValueError('Recovery belongs to a different game installation')
    profile = Path(launch['profile']).absolute()
    plain_directory(profile)
    if directory.parent != profile/'sessions':
        raise ValueError('Recovery is outside its recorded AP profile')
    marker = profile/'profile.json'
    if marker.is_symlink():
        raise ValueError('Linked AP profile metadata refused')
    identity = json.loads(marker.read_text(encoding='utf-8'))
    if identity['binding'] != launch['binding']:
        raise ValueError('Recovery AP identity differs')
    targets = [executable.parent/name for name in
               ('dinput8.dll', 'kh02-ap-ready.bin', 'kh02-ap-resume.bin')]
    mods = executable.parent.parent.parent/'Content/Paks/~mods'
    plain_directory(mods)
    targets.extend(mods/name for name in
                   ('KH02_AP_RestartPrototype_P.pak', 'TresGame-WindowsNoEditor_99_P.pak'))
    transaction = directory/'installation'
    plain_directory(transaction)
    if (transaction/'installation.json').is_symlink():
        raise ValueError('Linked installation manifest refused')
    Installation(transaction, require_closed=require_closed, allowed_targets=targets).restore()
    return directory
