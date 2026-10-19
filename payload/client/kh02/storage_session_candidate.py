"""Bind authenticated AP history to startup storage and existing receipt rules.

No process launching, file deletion, native effects or implicit save adoption.
The platform adapter supplies independently verified startup/process evidence;
the existing lifecycle owner still supplies native load and durable associations.
"""
from dataclasses import dataclass
import hashlib
import json
from pathlib import PureWindowsPath
from .ap_history_candidate import HistorySnapshot
from .received import DeliveryKey
from .save_association_candidate import resolve_disk_association

GAME='Kingdom Hearts 0.2 Birth by Sleep'
STEAM_BUILD='a5945af5041047f54db67d7667390f4dc90eee59d1c06459434ddab57d0f339c'

def scope_binding(key: DeliveryKey) -> str:
    if (type(key.seed_name) is not str or not key.seed_name or len(key.seed_name.encode('utf-8'))>4096
        or type(key.team) is not int or not 0<=key.team<2**32
        or type(key.slot) is not int or not 1<=key.slot<2**32):
        raise ValueError('Invalid authenticated AP identity')
    # Preserve exact seed identity; display-name/path sanitizing would collide.
    return hashlib.sha256(json.dumps([GAME,1,key.seed_name,key.team,key.slot],
        ensure_ascii=False,separators=(',',':')).encode('utf-8')).hexdigest()

@dataclass(frozen=True)
class StoragePreparation:
    key: DeliveryKey
    binding: str
    root: str
    executable: str
    executable_sha256: str
    prepared: int
    expires: int
    generation: str

@dataclass(frozen=True)
class StartupProof:
    pid: int
    created: int
    executable: str
    executable_sha256: str
    binding: str
    root: str
    ready: bool

def _path(value):
    p=PureWindowsPath(value)
    if not p.is_absolute() or '..' in p.parts:raise ValueError('Absolute fixed storage path required')
    return p

class StorageSession:
    def __init__(self, context):
        self.context=context
        self.preparation=None
        self.process=None
        self.faulted=False

    def snapshot(self) -> HistorySnapshot:
        snapshot=self.context.current_snapshot()
        if not self.context.snapshot_is_current(snapshot):raise ValueError('AP history is no longer current')
        if self.preparation and snapshot.key!=self.preparation.key:raise ValueError('Wrong AP session for fixed storage')
        return snapshot

    def prepare(self, *, root, executable, executable_sha256, prepared, expires,
                generation, running_games, publish):
        """Synchronous: no event-loop await between history check and publication.

        publish must back up/verify and install via the native storage adapter;
        if it fails this coordinator remains spent, never silently retried.
        running_games must include every detected KH0.2 storefront instance.
        """
        if self.preparation or self.faulted:raise ValueError('Session already prepared or faulted')
        snapshot=self.snapshot()
        if running_games():raise ValueError('Close KH0.2 normally before AP preparation')
        if executable_sha256!=STEAM_BUILD:raise ValueError('Unsupported storefront/build')
        _path(root);_path(executable)
        if type(prepared) is not int or type(expires) is not int or not 0<prepared<expires:raise ValueError('Invalid preparation interval')
        if type(generation) is not str or len(generation)!=32 or any(ch not in '0123456789abcdef' for ch in generation):raise ValueError('Fresh generation identity required')
        value=StoragePreparation(snapshot.key,scope_binding(snapshot.key),root,executable,executable_sha256,prepared,expires,generation)
        self.faulted=True
        if running_games() or not self.context.snapshot_is_current(snapshot):raise ValueError('Launch/history changed before preparation')
        publish(value,snapshot)
        self.preparation=value
        if running_games() or not self.context.snapshot_is_current(snapshot):raise ValueError('Launch/history changed during preparation; storage retained, effects disabled')
        self.faulted=False
        return value

    def admit_process(self, proof: StartupProof, *, now, running_games):
        p=self.preparation
        if p is None or self.faulted:raise ValueError('No valid preparation')
        games=tuple(running_games())
        if len(games)!=1 or games[0]!=proof:raise ValueError('Ambiguous or mismatched running game')
        if (type(proof.pid) is not int or proof.pid<=0 or type(proof.created) is not int
            or not p.prepared<proof.created<=now<p.expires
            or proof.ready is not True or proof.binding!=p.binding
            or _path(proof.root)!=_path(p.root) or _path(proof.executable)!=_path(p.executable)
            or proof.executable_sha256!=p.executable_sha256):
            raise ValueError('Game does not prove the prepared storage/session')
        if self.process is not None and proof!=self.process:raise ValueError('Prepared process identity cannot change')
        # Storage stays usable if AP disconnected. Effects need fresh history
        # separately; no network loss changes the game's pinned root.
        self.process=proof
        return proof

    def resolve_loaded_record(self, *, proof, record, associations, supported,
                              native_load_current):
        if self.faulted or self.process is None or proof!=self.process:raise ValueError('Unadmitted game process')
        snapshot=self.snapshot()
        if not native_load_current():raise ValueError('Native load is not current')
        result=resolve_disk_association(snapshot.key,record,associations,
                                        snapshot.received_items(),supported)
        if not self.context.snapshot_is_current(snapshot) or not native_load_current():raise ValueError('AP/native load changed during resolution')
        return result
