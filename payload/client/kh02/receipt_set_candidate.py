"""Bounded immutable receipt-set wire candidate; no native delivery or persistence.

An encoded proposal is not evidence that its effect occurred. Native publication
must retain the exact set and select its revision only after verified application.
"""
from dataclasses import dataclass
import hashlib
import struct

from .received import DeliveryKey, ReceivedItem

HEADER = struct.Struct('<8sIIQQQIIII')
ITEM = struct.Struct('<QQQ')
MAGIC, MAX_ITEMS, MAX_SEED = b'KH02RSET', 4096, 4096


@dataclass(frozen=True)
class ReceiptSet:
    key: DeliveryKey
    owner_cookie: int
    load_id: int  # Validated native load identity, not a readiness polling epoch.
    revision: int
    items: tuple[ReceivedItem, ...]

    @property
    def digest(self):
        return hashlib.sha256(encode(self)).hexdigest()


def _uint(value, bits, *, nonzero=False):
    if type(value) is not int or not (1 if nonzero else 0) <= value < 2**bits:
        raise ValueError('Integer outside wire range')


def encode(value: ReceiptSet) -> bytes:
    for n in (value.owner_cookie, value.load_id, value.revision):
        _uint(n, 64, nonzero=True)
    for n in (value.key.team, value.key.slot):
        _uint(n, 32)
    if type(value.key.seed_name) is not str:
        raise ValueError('Seed identity must be text')
    seed = value.key.seed_name.encode('utf-8')
    if not 0 < len(seed) <= MAX_SEED or type(value.items) is not tuple or len(value.items) > MAX_ITEMS:
        raise ValueError('Invalid or excessive receipt-set size')
    previous = -1
    body = bytearray()
    for item in value.items:
        for n in (item.index, item.item_id, item.player):
            _uint(n, 64)
        if item.index <= previous:
            raise ValueError('Receipt identities must be unique and ordered')
        previous = item.index
        body += ITEM.pack(item.index, item.item_id, item.player)
    return HEADER.pack(MAGIC, 1, len(value.items), value.owner_cookie, value.load_id,
                       value.revision, value.key.team, value.key.slot, len(seed), 0)+seed+body


def decode(data: bytes) -> ReceiptSet:
    if type(data) is not bytes or not HEADER.size <= len(data) <= HEADER.size+MAX_SEED+ITEM.size*MAX_ITEMS:
        raise ValueError('Invalid receipt-set framing')
    magic, version, count, cookie, load, revision, team, slot, seed_len, reserved = HEADER.unpack_from(data)
    if (magic != MAGIC or version != 1 or reserved or count > MAX_ITEMS or
            not 0 < seed_len <= MAX_SEED or len(data) != HEADER.size+seed_len+ITEM.size*count):
        raise ValueError('Unsupported or inconsistent receipt-set framing')
    seed = data[HEADER.size:HEADER.size+seed_len].decode('utf-8')
    items = tuple(ReceivedItem(*ITEM.unpack_from(data, offset))
                  for offset in range(HEADER.size+seed_len, len(data), ITEM.size))
    value = ReceiptSet(DeliveryKey(seed, team, slot), cookie, load, revision, items)
    if encode(value) != data:
        raise ValueError('Noncanonical receipt set')
    return value


def validate(value, key, cookie, load_id, history, supported):
    encode(value)
    if (value.key, value.owner_cookie, value.load_id) != (key, cookie, load_id):
        raise ValueError('Receipt set belongs to another AP slot or native load')
    if any(type(item.index) is not int or item.index != index for index, item in enumerate(history)):
        raise ValueError('Complete original server history required')
    for item in value.items:
        if item.index >= len(history) or history[item.index] != item or item.item_id not in supported:
            raise ValueError('Receipt identity conflicts with complete server history')


def propose_next(value, item, history, supported):
    """Prepare the next immutable set BEFORE an effect; does not commit it."""
    validate(value, value.key, value.owner_cookie, value.load_id, history, supported)
    if (type(item.index) is not int or item.index < 0 or item.index >= len(history) or
            history[item.index] != item or item.item_id not in supported or
            any(old.index == item.index for old in value.items)):
        raise ValueError('Item is not an unapplied supported receipt')
    proposal = ReceiptSet(value.key, value.owner_cookie, value.load_id, value.revision+1,
                          tuple(sorted(value.items+(item,), key=lambda row: row.index)))
    encode(proposal)  # Enforce capacity/overflow before admitting any effect.
    return proposal
