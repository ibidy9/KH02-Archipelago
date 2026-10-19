"""Bounded history payload for owner-thread transport command 4.

Encoding is not authorization. The caller must use CoordinatedHistory with its
authenticated snapshot and verify the bound transport response sequence. This
command changes availability only, never the applied receipt set.
"""
import struct
from .coordinated_history_candidate import _items
from .received import DeliveryKey

HEADER = struct.Struct('<IIIIQQIIII')
MAGIC = 0x4b484843
COMMAND = 4
MAX_SIZE = 48 + 4096 * 24 + 4096


def encode_history(key: DeliveryKey, cookie: int, load_id: int, history) -> bytes:
    if not isinstance(key, DeliveryKey):
        raise ValueError('Expected delivery key')
    for value, bits, minimum in ((cookie, 64, 1), (load_id, 64, 1),
                                 (key.team, 32, 0), (key.slot, 32, 0)):
        if type(value) is not int or not minimum <= value < 2**bits:
            raise ValueError('Invalid history command scope')
    if not isinstance(key.seed_name, str):
        raise ValueError('Expected seed name')
    seed = key.seed_name.encode('utf-8')
    if not 1 <= len(seed) <= 4096:
        raise ValueError('Invalid seed byte length')
    items = _items(history)
    identities = b''.join(struct.pack('<QQQ', x.index, x.item_id, x.player) for x in items)
    return HEADER.pack(MAGIC, 1, HEADER.size + len(identities) + len(seed),
                       len(items), cookie, load_id, key.team, key.slot, len(seed), 0) + identities + seed
