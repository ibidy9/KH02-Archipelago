"""Goal proof/outbox candidate; no game reads, writes or implicit save adoption.

The caller must supply a current shared-owner admission for every observation.
Witnessed Objective42 1->2 or scoped Tide battle-bonus 0->1 is accepted. Existing completed
records cannot establish a new proof. Reconnect recovery uses the scoped proof.
Demon Tide uses v69 evidence and the exact-build BattleBonus flag audit.
"""
from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path

from .goal_access_candidate import Goal
from .received import DeliveryKey


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode('utf-8')


@dataclass(frozen=True)
class GoalAdmission:
    key: DeliveryKey
    generation: str  # Explicit associated AP file generation, not save-slot number.
    process: str     # Validated process identity, not PID alone.
    owner: int
    epoch: int

    def validate(self):
        if (not isinstance(self.key, DeliveryKey) or not self.key.seed_name
                or type(self.key.team) is not int or self.key.team < 0
                or type(self.key.slot) is not int or self.key.slot <= 0
                or any(type(v) is not str or not v for v in (self.generation, self.process))
                or any(type(v) is not int or v <= 0 for v in (self.owner, self.epoch))):
            raise ValueError('Explicit associated AP generation and current owner required')


PREDICATES = {Goal.BOSS_RUSH.value: ('objective42-transition-1-to-2-v68', 1, 2),
              Goal.DEMON_TIDE.value: ('dw10-battle-bonus-bit47-transition-v69', 0, 1)}


class GoalProofStore:
    """Immutable one-proof-per-scope journal. Never alters game saves.

    Hashes detect damage, not malicious edits. This is local recovery evidence,
    not a security boundary against a user modifying their own client.
    """
    def __init__(self, root, key, goal):
        if not isinstance(goal, Goal):
            raise ValueError('Explicit selected goal required')
        self.scope = dict(key=asdict(key), goal=goal.value)
        self.path = Path(root) / (hashlib.sha256(canonical(self.scope)).hexdigest()+'.json')

    def load(self):
        if not self.path.exists():
            return None
        envelope = json.loads(self.path.read_text(encoding='utf-8'))
        proof = envelope['proof']
        if (envelope['sha256'] != hashlib.sha256(canonical(proof)).hexdigest()
                or proof['version'] != 1 or proof['scope'] != self.scope
                or (proof['predicate'], proof['before'], proof['after']) != PREDICATES[self.scope['goal']]):
            raise ValueError('Goal proof is damaged or belongs to another scope')
        a = proof['admission']
        admission = GoalAdmission(DeliveryKey(**a['key']), a['generation'],
                                  a['process'], a['owner'], a['epoch'])
        admission.validate()
        if asdict(admission.key) != self.scope['key']:
            raise ValueError('Goal admission scope differs')
        return proof

    def commit(self, admission):
        admission.validate()
        if asdict(admission.key) != self.scope['key']:
            raise ValueError('Unsupported goal or mismatched admission')
        if self.load() is not None:
            return
        proof = dict(version=1, scope=self.scope, admission=asdict(admission),
                     predicate=PREDICATES[self.scope['goal']][0],
                     before=PREDICATES[self.scope['goal']][1], after=PREDICATES[self.scope['goal']][2])
        envelope = dict(proof=proof, sha256=hashlib.sha256(canonical(proof)).hexdigest())
        self.path.parent.mkdir(parents=True, exist_ok=True)
        # Exclusive staging and hard-link publication: never replace an existing
        # proof, even if another observer commits concurrently. Retain staging
        # files for recovery; a partial staging file is never treated as a proof.
        import uuid
        stage = self.path.with_suffix('.'+uuid.uuid4().hex+'.pending')
        with stage.open('xb') as stream:
            stream.write(canonical(envelope))
            stream.flush()
            os.fsync(stream.fileno())
        try:
            os.link(stage, self.path)
        except FileExistsError:
            pass
        self.load()


class GoalCompletion:
    def __init__(self, store):
        self.store = store
        self.baseline = None

    def invalidate(self):
        self.baseline = None

    def observe(self, admission, objective42, *, tide_bonus=None, map_name=None, is_current):
        """Synchronous validation; caller must invalidate on read/lifecycle gaps."""
        try:
            return self._observe(admission, objective42, tide_bonus=tide_bonus,
                                 map_name=map_name, is_current=is_current)
        except Exception:
            self.invalidate()
            raise

    def _observe(self, admission, objective42, *, tide_bonus, map_name, is_current):
        admission.validate()
        if (asdict(admission.key) != self.store.scope['key']
                or not is_current(admission)):
            self.invalidate()
            raise ValueError('Goal observation has no current matching admission')
        if type(objective42) is not int or objective42 not in (0, 1, 2):
            self.invalidate()
            raise ValueError('Invalid objective state')
        state = objective42
        if self.store.scope['goal'] == Goal.DEMON_TIDE.value:
            # Full byte at structured+3205, not a supplied success boolean.
            if type(tide_bonus) is not int or not 0 <= tide_bonus <= 255:
                self.invalidate()
                return False
            if map_name != '/Game/Levels/dw/dw_10/dw_10':
                self.invalidate()
                return False
            state = (tide_bonus >> 7) & 1
        _, incomplete, complete = PREDICATES[self.store.scope['goal']]
        if self.store.load() is not None:
            return True
        if state == incomplete:
            self.baseline = admission
            return False
        if state == complete and self.baseline == admission:
            if not is_current(admission):
                self.invalidate()
                raise ValueError('Goal admission expired before commit')
            self.store.commit(admission)
            self.invalidate()
            return True
        self.invalidate()
        return False

    async def report(self, context):
        """Persisted proof may be resent after reconnect; no item/check effects.

        Use the captured authenticated socket so a connection switch cannot send
        this proof into a different room. Successful send is not server ACK.
        """
        if self.store.load() is None:
            return False
        snapshot = context.current_snapshot()
        if (asdict(snapshot.key) != self.store.scope['key']
                or not context.snapshot_is_current(snapshot)):
            raise ValueError('Goal reporting requires matching authenticated room')
        socket = snapshot.connection.socket
        await socket.send(json.dumps([dict(cmd='StatusUpdate', status=30)]))
        return True
