"""Prepare real AP fresh/resume payloads without modifying any game save."""
import hashlib
from pathlib import Path
import secrets
import struct

from .autosave_resume_v129_candidate import encode_setup
from .fresh_transport_host_candidate import FreshPreparation
from .mickey_entry_v172_candidate import prepare as prepare_fresh, supported_effects
from .mickey_receipts_v171 import build_policy
from .multi_record_journal_candidate import MultiRecordJournal
from .receipt_set_candidate import ReceiptSet, encode
from .save_association_candidate import resolve_disk_association

RAW_SIZE, RECORD_SIZE, RECORD_OFFSET = 0x1725b0, 0x3b40, 0xb0


def resume_records(raw, associations, *, key, history, cookie, target=1):
    """Seed all current associated records, independently of initial-load target.

    Unknown secondary records remain unknown. Conflicting associated records
    fail preparation; never silently discard their receipt evidence.
    """
    if (type(raw) is not bytes or len(raw) != RAW_SIZE or
            raw[:16] != struct.pack('<4sIII', b'SAVE', 1, RAW_SIZE, 0)):
        raise ValueError('Verified KH02 decoded container required')
    if type(target) is not int or target not in (0, 1):
        raise ValueError('Initial load must use autosave or manual slot 1')
    history = tuple(history)
    build_policy(history)
    supported = frozenset(supported_effects())
    entries = {}
    for slot in (target, *(n for n in range(100) if n != target)):
        start = RECORD_OFFSET + slot * RECORD_SIZE
        record = raw[start:start+RECORD_SIZE]
        result = resolve_disk_association(key, record, associations, history, supported)
        if result.checkpoint is None:
            if slot == target:
                raise ValueError(f'Slot {target} is not associated with this AP seed: {result.reason}')
            if any(a.key == key and a.record == record for a in associations):
                raise ValueError(f'Slot {slot} has conflicting AP evidence: {result.reason}')
            continue
        entries[record] = encode(ReceiptSet(key, cookie, 1, 1, result.checkpoint.included))
    return tuple(entries.items())


def read_associations(paths):
    """Audit each closed journal; do not impose the old 64-history-record cap."""
    rows = []
    for path in paths:
        journal = MultiRecordJournal(Path(path), readonly=True)
        try:
            rows.extend(journal.read_all())
        finally:
            journal.close()
    return tuple(rows)


def prepare(session, storage, *, journal_path, runtime_start, raw=None,
            associations=(), target=1, cookie=None, grant_nonce=None):
    cookie = (secrets.randbits(64) or 1) if cookie is None else cookie
    grant_nonce = (secrets.randbits(64) or 1) if grant_nonce is None else grant_nonce
    xp, admitted = session.context.xp_admission.current(session.context)
    mickey, paired = session.context.mickey_admission.current(session.context)
    if xp.mickey != mickey or admitted != paired or admitted.key != storage.key:
        raise ValueError("XP preparation admission differs")
    if raw is None:
        result = prepare_fresh(session, storage, cookie=cookie, grant_nonce=grant_nonce,
                             journal_path=str(journal_path), runtime_start=runtime_start)
        if session.context.xp_admission.current(session.context) != (xp, admitted):
            raise ValueError("XP admission changed during fresh preparation")
        return result
    if type(grant_nonce) is not int or not 0 < grant_nonce < 2**64:
        raise ValueError('Nonzero resume grant nonce required')
    mickey, snapshot = session.context.mickey_admission.current(session.context)
    config, current = session.context.goal_admission.current(session.context)
    if (mickey.goal != config or snapshot != current or snapshot.key != storage.key
            or config.starting_world != runtime_start):
        raise ValueError('Prepared seed, goal, Mickey, or starting world differs')
    history = snapshot.received_items()
    entries = resume_records(raw, tuple(associations), key=storage.key,
                             history=history, cookie=cookie, target=target)
    payload = bytearray(encode_setup(key=storage.key, history=history,
        supported=supported_effects(), cookie=cookie, generation=storage.generation,
        journal_path=str(journal_path), entries=entries, target=target,
        publication_prefix=secrets.token_hex(16)))
    struct.pack_into('<I', payload, 12, 6)
    struct.pack_into('<Q', payload, 120, grant_nonce)
    if not session.context.snapshot_is_current(snapshot):
        raise ValueError('AP connection/history changed during resume preparation')
    if session.context.mickey_admission.current(session.context) != (mickey, snapshot):
        raise ValueError('Mickey admission changed during resume preparation')
    if session.context.xp_admission.current(session.context) != (xp, admitted):
        raise ValueError("XP admission changed during resume preparation")
    payload = bytes(payload)
    return FreshPreparation(storage, payload, hashlib.sha256(payload).digest(), cookie, history)
