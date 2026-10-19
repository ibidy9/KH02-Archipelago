"""Immutable authenticated XP configuration; schema3 remains unchanged."""
from dataclasses import dataclass
from .mickey_slot_v172_candidate import parse as mickey_parse, MickeyAdmission

@dataclass(frozen=True)
class XPSlot:
    mickey: object
    halves: int

def parse(value):
    if type(value) is not dict:
        raise ValueError('XP slot data must be an object')
    base = dict(value)
    if type(base.get('schema_version')) is not int or base['schema_version'] != 4:
        raise ValueError('XP configuration requires schema4')
    halves = base.pop('xp_multiplier_halves', None)
    if type(halves) is not int or halves not in (2, 3, 4, 6, 8):
        raise ValueError('Unsupported XP multiplier')
    base['schema_version'] = 3
    return XPSlot(mickey_parse(base), halves)

class XPAdmission(MickeyAdmission):
    def connected(self, connection, key, slot_data):
        self.disconnect()
        if connection is None or key != self.key:
            raise ValueError('XP room differs')
        configuration = parse(slot_data)
        if self.configuration is not None and configuration != self.configuration:
            raise ValueError('Seed XP configuration changed')
        self.configuration = configuration
        self.connection = connection
