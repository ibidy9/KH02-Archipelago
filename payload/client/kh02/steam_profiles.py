"""Seed-scoped storage registry. Never adopts, deletes, or edits game saves."""
from dataclasses import asdict
import json
from pathlib import Path
import re
import secrets

from .storage_session_candidate import scope_binding


def profile_path(root, key):
    binding = scope_binding(key)
    label = re.sub(r'[^A-Za-z0-9_-]', '_', key.seed_name)[:24] or 'Seed'
    return Path(root).resolve() / f'{label}-P{key.slot}-{binding[:16]}'


def plain_directory(path):
    """Reject symlinks/junctions in existing ancestors of a storage path."""
    path = Path(path).absolute()
    for part in (path, *path.parents):
        if part.is_symlink() or (hasattr(part, 'is_junction') and part.is_junction()):
            raise ValueError('AP storage must not use links or junctions')
        if part.exists() and not part.is_dir():
            raise ValueError('AP storage path is not a directory')


def open_profile(root, key, slot_data, *, player_name):
    from .mickey_slot_v172_candidate import parse
    parse(slot_data)
    plain_directory(root)
    path = profile_path(root, key)
    plain_directory(path)
    marker = path / 'profile.json'
    identity = dict(version=1, scope=asdict(key), binding=scope_binding(key), slot_data=slot_data)
    if path.exists():
        if not marker.is_file() or marker.is_symlink():
            raise ValueError('Existing folder lacks AP ownership; no save adoption')
        value = json.loads(marker.read_text(encoding='utf-8'))
        if any(value.get(k) != v for k, v in identity.items()):
            raise ValueError('AP profile identity or seed settings differ')
        generation = value.get('generation')
        if not isinstance(generation, str) or not re.fullmatch('[0-9a-f]{32}', generation) or int(generation, 16) == 0:
            raise ValueError('Invalid AP profile generation')
    else:
        path.mkdir(parents=True, exist_ok=False)
        value = dict(identity, generation=secrets.token_hex(16),
                     label=f'Archipelago — {key.seed_name} — {player_name}')
        # A failed write leaves a refused folder, never an implicitly owned one.
        with marker.open('x', encoding='utf-8') as stream:
            json.dump(value, stream, ensure_ascii=False, indent=2)
    for name in ('saves', 'claims', 'receipts', 'sessions', 'shared-client'):
        folder = path / name
        plain_directory(folder)
        folder.mkdir(exist_ok=True)
    return path, value
