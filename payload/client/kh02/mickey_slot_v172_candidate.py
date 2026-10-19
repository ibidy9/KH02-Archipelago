"""Strict authenticated slot configuration for Mickey; no game side effects."""
from dataclasses import dataclass
from .goal_session_v89_candidate import parse_goal_slot_data

@dataclass(frozen=True)
class MickeySlot:
    goal: object
    mode: str

def parse(value):
    if type(value) is not dict:
        raise ValueError('Mickey slot data must be an object')
    base = dict(value)
    if type(base.get('schema_version')) is not int or base['schema_version'] != 3:
        raise ValueError('Mickey requires explicit schema3')
    mode = base.pop('mickey', None)
    if type(mode) is not str or mode not in ('vanilla', 'randomized_depths', 'randomized_all_worlds'):
        raise ValueError('Unknown Mickey mode')
    base['schema_version'] = 2
    return MickeySlot(parse_goal_slot_data(base), mode)

class MickeyAdmission:
    def __init__(self, key):
        self.key=key; self.connection=None; self.configuration=None

    def disconnect(self):
        self.connection=None

    def connected(self, connection, key, slot_data):
        self.disconnect()
        if connection is None or key != self.key:
            raise ValueError('Mickey room differs')
        configuration=parse(slot_data)
        if self.configuration is not None and configuration != self.configuration:
            raise ValueError('Seed Mickey configuration changed')
        self.configuration=configuration; self.connection=connection

    def current(self, context):
        snapshot=context.current_snapshot()
        if (self.configuration is None or self.connection is None or
            snapshot.key!=self.key or snapshot.connection is not self.connection or
            not context.snapshot_is_current(snapshot)):
            raise ValueError('Mickey requires matching authenticated history')
        return self.configuration,snapshot
