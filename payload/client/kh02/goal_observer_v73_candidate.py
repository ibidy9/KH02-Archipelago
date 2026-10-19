"""Read-only v72 publication and receipt-bound journal candidate.

Explicit construction only; not installed in a production watcher yet.
No process-memory reads, game writes, launches, or goal sends.
"""
import mmap
import struct

from .goal_access_candidate import Goal
from .goal_completion_v2_candidate import GoalAdmission, GoalProofStore
from .goal_witness_v71_candidate import WIRE, WitnessBinding, journal_witness

HEADER = struct.Struct('<4IQII')
SIZE = 176
MAGIC = 0x3237474b


class PublicationUnavailable(ValueError):
    """A snapshot is incomplete or changed; a later tick may read it again."""


def stable_publication(first, second):
    if type(first) is not bytes or len(first) != SIZE or first != second:
        raise PublicationUnavailable('Goal publication changed during read')
    magic, version, size, sequence, tick, admitted, reserved = HEADER.unpack_from(first)
    if ((magic, version, size) != (MAGIC, 1, SIZE) or not sequence
            or sequence & 1 or not tick or admitted not in (0, 1) or reserved):
        raise PublicationUnavailable('Goal publication is not initialized and stable')
    return first[32:104], first[104:176]


class GoalObserver:
    def __init__(self, coordinator, journal_root):
        # Read the goal from prepared production slot data, never a mapping.
        owner = coordinator.transport
        prepared = coordinator.prepared
        if (owner is None or prepared is None or coordinator.process != owner.proof
                or prepared.storage != owner.prepared.storage):
            raise ValueError('Attached prepared production session required')
        owner._check_session()
        self.owner = owner
        self.proof = owner.proof
        self.storage = prepared.storage
        self.goal = Goal(prepared.slot_data.goal)
        self.store = GoalProofStore(journal_root, self.storage.key, self.goal)
        self.bindings = {}
        self.memory = mmap.mmap(-1, SIZE,
            tagname=f'Local\\KH02APGoalObservationV72-{self.proof.pid}')
        if self.memory[:] != bytes(SIZE):
            self.memory.close()
            raise RuntimeError('Existing goal observer preserved')
        self.memory[:16] = struct.pack('<4I', MAGIC, 1, SIZE, 0)

    def _process_current(self):
        if self.owner.proof != self.proof or self.owner.game_exited():
            raise ValueError('Original goal observation process is unavailable')

    async def bind(self):
        """Capture identity through the proven authenticated receipt probe."""
        self._process_current()
        self.owner._check_session()
        before = self.owner.meter_access_status()
        observed = await self.owner.probe()
        self._process_current()
        self.owner._check_session()
        after = self.owner.meter_access_status()
        if observed is None:
            return False
        epoch, receipt = observed
        if (before['uncertain'] or after['uncertain'] or not after['owner']
                or (before['owner'], before['epoch']) != (after['owner'], epoch)
                or after['epoch'] != epoch):
            return False
        if receipt.key != self.storage.key or receipt.owner_cookie != self.owner.prepared.cookie:
            raise ValueError('Receipt scope differs from prepared storage')
        admission = GoalAdmission(self.storage.key, self.storage.generation,
            f'{self.proof.pid}:{self.proof.created}', after['owner'], epoch)
        binding = WitnessBinding(admission, receipt.owner_cookie, receipt.load_id, self.goal)
        binding.validate()
        key = (binding.cookie, epoch, binding.load_id, after['owner'])
        if key not in self.bindings and len(self.bindings) >= 128:
            raise RuntimeError('Goal binding capacity reached; retained proofs preserved')
        self.bindings[key] = binding
        return True

    def poll(self):
        # A completed original proof is valid after its gameplay owner retires.
        # Do not demand that it match the NEW current owner or network epoch.
        self._process_current()
        first = self.memory[:]
        parts = stable_publication(first, self.memory[:])
        self._process_current()
        raw = parts[0 if self.goal == Goal.BOSS_RUSH else 1]
        fields = WIRE.unpack(raw)
        if fields == (0,)*10:
            return False
        binding = self.bindings.get(fields[:4])
        if binding is None:
            return False
        return journal_witness(raw, binding, self.store)

    def close(self):
        self.memory.close()
