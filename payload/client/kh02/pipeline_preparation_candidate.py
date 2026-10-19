"""Exclusive offline preparation for an isolated publication test, never AP adoption.

No process API, live attachment, save modification, restore, or grant. An empty
new synthetic scope says nothing about the AP history of the copied game save.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import secrets
import shutil
import uuid

from .multi_record_journal_candidate import initialize, MultiRecordJournal
from .received import DeliveryKey
from .receipt_set_candidate import ReceiptSet, encode
from .pipeline_profile_candidate import compare

SIZE = 0x1725b0
SCHEMA = 'kh02-isolated-publication-preparation-v1'


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(directory: Path) -> dict[str, str]:
    directory = directory.resolve(strict=True)
    result = {}
    for path in sorted(directory.rglob('*')):
        if path.is_symlink() or path.is_junction():
            raise ValueError('Linked profile entries are refused')
        if path.is_file():
            if not path.resolve().is_relative_to(directory):
                raise ValueError('Profile entry escapes its root')
            result[path.relative_to(directory).as_posix()] = digest(path)
    if not result:
        raise ValueError('Empty profile is not a baseline')
    return result


def _raw(path: Path) -> bytes:
    raw = path.read_bytes()
    if (len(raw) != SIZE or raw[:4] != b'SAVE' or
        tuple(int.from_bytes(raw[i:i+4], 'little') for i in (4, 8, 12)) != (1, SIZE, 0)):
        raise ValueError('Invalid decoded full-container baseline')
    return raw


def prepare(destination: Path, *, profile: Path, expected_profile: dict[str, str],
            decoded: Path, expected_decoded_hash: str, assets: dict[Path, str], profile_policy=None) -> Path:
    """Caller establishes game-closed state before this exclusive preparation.

    All expected hashes come from independently verified evidence. Destination
    must not exist; partial failures remain visible and are never reused.
    """
    profile = profile.resolve(strict=True)
    destination = destination.resolve()
    if destination.is_relative_to(profile) or profile.is_relative_to(destination):
        raise ValueError('Preparation and live profile must be disjoint')
    if destination.exists():
        raise FileExistsError('Preparation directory already exists; never reuse it')
    if inventory(profile) != expected_profile:
        raise ValueError('Live profile differs from the independently verified closed baseline')
    compare(expected_profile,expected_profile,profile_policy)
    raw = _raw(decoded)
    if hashlib.sha256(raw).hexdigest() != expected_decoded_hash.lower():
        raise ValueError('Decoded baseline hash mismatch')
    checked = []
    for path, expected in assets.items():
        path = path.resolve(strict=True)
        if digest(path) != expected.lower():
            raise ValueError(f'Artifact hash mismatch: {path.name}')
        checked.append({'path': str(path), 'sha256': expected.lower()})
    if not checked:
        raise ValueError('Pinned artifacts are required')
    destination.mkdir(parents=True, exist_ok=False)
    copy = destination / 'baseline-profile'
    shutil.copytree(profile, copy)
    if inventory(copy) != expected_profile or inventory(profile) != expected_profile:
        raise ValueError('Profile changed during verified copy')
    (destination / 'baseline.raw').write_bytes(raw)
    nonce = uuid.uuid4().hex
    seed = 'kh02-publication-probe-' + nonce
    cookie = secrets.randbits(64) or 1
    receipt = ReceiptSet(DeliveryKey(seed, 0, 1), cookie, 1, 1, ())
    (destination / 'synthetic-receipts.bin').write_bytes(encode(receipt))
    journal = destination / 'isolated.sqlite'
    initialize(journal)
    # Audit reference schema/links before handing it to any future native reader.
    opened = MultiRecordJournal(journal)
    try:
        if opened.read_all() or opened.db.execute('SELECT count(*) FROM publications').fetchone()[0]:
            raise ValueError('Fresh journal is not empty')
    finally:
        opened.close()
    manifest = {
        'schema': SCHEMA, 'live_ready': False, 'authorizes_delivery': False,
        'scope_kind': 'new-synthetic-test-only', 'real_save_membership': 'unknown',
        'empty_scope_reason': 'No effects have been submitted in this newly created test namespace.',
        'seed': seed, 'team': 0, 'slot': 1, 'owner_cookie': cookie, 'test_load_epoch': 1,
        'prefix': nonce, 'shutdown': 'retain-barrier-until-original-process-exit',
        'profile_path': str(profile), 'profile_files': expected_profile,
        'assets': checked,
        'files': {name: digest(destination / name) for name in
                  ('baseline.raw', 'synthetic-receipts.bin', 'isolated.sqlite')},
        'protected_manual_slots': [56, 62, 64, 68],
        'missing_live_requirements': ['native-bootstrap-and-transport-acceptance',
                                     'complete-release-recovery-and-live-preflight'],
    }
    if profile_policy is not None:manifest['profile_policy']=profile_policy
    path = destination / 'preparation.json'
    path.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    (destination / 'preparation.sha256').write_text(digest(path) + '\n', encoding='ascii')
    validate(path)
    return path


def validate(path: Path) -> dict:
    """Before first use only: any journal change makes this preparation spent.

    A future live runner must retain this original manifest; never rewrite its
    journal hash to make an already-used directory appear fresh.
    """
    from .receipt_set_candidate import decode
    path = path.resolve(strict=True)
    if path.name != 'preparation.json' or digest(path) != path.with_suffix('.sha256').read_text().strip():
        raise ValueError('Preparation manifest seal mismatch')
    value = json.loads(path.read_text())
    if (value.get('schema') != SCHEMA or value.get('live_ready') is not False or
        value.get('authorizes_delivery') is not False or
        value.get('scope_kind') != 'new-synthetic-test-only' or
        value.get('real_save_membership') != 'unknown' or
        value.get('shutdown') != 'retain-barrier-until-original-process-exit' or
        value.get('protected_manual_slots') != [56, 62, 64, 68]):
        raise ValueError('Invalid isolated preparation policy')
    prefix = value.get('prefix')
    if (type(prefix) is not str or len(prefix) != 32 or any(ch not in '0123456789abcdef' for ch in prefix) or
        value.get('seed') != 'kh02-publication-probe-' + prefix or
        type(value.get('owner_cookie')) is not int or not 0 < value['owner_cookie'] < 2**64 or
        (value.get('team'), value.get('slot'), value.get('test_load_epoch')) != (0, 1, 1)):
        raise ValueError('Invalid synthetic scope')
    if set(value['files']) != {'baseline.raw', 'synthetic-receipts.bin', 'isolated.sqlite'}:
        raise ValueError('Invalid preparation file list')
    for name, expected in value['files'].items():
        target = path.parent / name
        if target.is_symlink() or target.resolve().parent != path.parent or digest(target) != expected:
            raise ValueError('Prepared input changed')
    _raw(path.parent / 'baseline.raw')
    receipt = decode((path.parent / 'synthetic-receipts.bin').read_bytes())
    expected = ReceiptSet(DeliveryKey(value['seed'], 0, 1), value['owner_cookie'], 1, 1, ())
    if receipt != expected:
        raise ValueError('Synthetic receipt scope mismatch')
    compare(value['profile_files'],inventory(Path(value['profile_path'])),value.get('profile_policy'))
    if inventory(path.parent / 'baseline-profile') != value['profile_files']:
        raise ValueError('Protected profile or baseline copy changed')
    if not value['assets'] or any(digest(Path(a['path'])) != a['sha256'] for a in value['assets']):
        raise ValueError('Prepared artifact changed')
    journal = MultiRecordJournal(path.parent / 'isolated.sqlite')
    try:
        if journal.read_all() or journal.db.execute('SELECT count(*) FROM publications').fetchone()[0]:
            raise ValueError('Preparation already used')
    finally:
        journal.close()
    return value
