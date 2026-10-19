"""Strict decoder for the offline native receipt-bearing save bundle.

Decoding is not proof of native construction, load association or durable save.
The caller must already own a complete copy while its native lease remains held.
"""
from dataclasses import dataclass
import struct

from .received import DeliveryKey
from .receipt_set_candidate import ReceiptSet, decode as decode_set, validate

CONTAINER_SIZE = 0x1725B0
HEADER = struct.Struct('<QQQQIIIIII100Q')
ENTRY = struct.Struct('<QII')
CONTAINER_OFFSET = HEADER.size + 64 * ENTRY.size
FIXED_SIZE = CONTAINER_OFFSET + CONTAINER_SIZE
MAX_SET_SIZE = 102456


@dataclass(frozen=True)
class ReceiptBundle:
    instance: int
    serial: int
    key: DeliveryKey
    owner_cookie: int
    load_id: int
    revisions: tuple[int, ...]
    sets: tuple[ReceiptSet, ...]
    container: bytes


def decode(data: bytes) -> ReceiptBundle:
    if type(data) is not bytes or not FIXED_SIZE + 1 <= len(data) <= FIXED_SIZE + 4096 + 64 * MAX_SET_SIZE:
        raise ValueError('Invalid bundle size or ownership')
    instance, serial, cookie, load_id, size, count, team, slot, seed_size, seed_offset, *revisions = HEADER.unpack_from(data)
    if (not all((instance, serial, cookie, load_id)) or size != len(data) or count > 64 or
            not 1 <= seed_size <= 4096 or seed_offset != FIXED_SIZE or seed_offset + seed_size > len(data)):
        raise ValueError('Invalid bundle header')
    seed = data[seed_offset:seed_offset + seed_size].decode('utf-8')
    key = DeliveryKey(seed, team, slot)
    sets = []
    seen = set()
    cursor = seed_offset + seed_size
    for index in range(64):
        revision, offset, set_size = ENTRY.unpack_from(data, HEADER.size + index * ENTRY.size)
        if index >= count:
            if (revision, offset, set_size) != (0, 0, 0):
                raise ValueError('Unused bundle descriptor is not empty')
            continue
        if (not revision or revision in seen or offset != cursor or not 56 <= set_size <= MAX_SET_SIZE or
                offset + set_size > len(data)):
            raise ValueError('Invalid or overlapping receipt descriptor')
        value = decode_set(data[offset:offset + set_size])
        if (value.key, value.owner_cookie, value.load_id, value.revision) != (key, cookie, load_id, revision):
            raise ValueError('Receipt scope differs from save bundle')
        sets.append(value)
        seen.add(revision)
        cursor += set_size
    if cursor != len(data) or seen != {revision for revision in revisions if revision}:
        raise ValueError('Unresolved record membership or unused receipt proposal')
    container = data[CONTAINER_OFFSET:FIXED_SIZE]
    if container[:16] != struct.pack('<4sIII', b'SAVE', 1, CONTAINER_SIZE, 0):
        raise ValueError('Invalid native container header')
    return ReceiptBundle(instance, serial, key, cookie, load_id, tuple(revisions), tuple(sets), container)


def validate_history(bundle: ReceiptBundle, history, supported):
    """Recheck all included receipts against complete original AP server history."""
    for value in bundle.sets:
        validate(value, bundle.key, bundle.owner_cookie, bundle.load_id, history, supported)
