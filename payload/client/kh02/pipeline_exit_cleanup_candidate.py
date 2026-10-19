"""Controlled publication experiment: retain the barrier until verified exit.

Separate from normal route cleanup. A native retained-barrier acknowledgment is
NOT an uninstallation receipt. This policy never removes a hook or releases a
mapping while the original verified process remains alive.
"""
from .controller_cleanup_candidate import CleanupReceipt, CleanupUncertain

RETAINED_BARRIER = 8


class PipelineExitCleanup:
    def __init__(self, transport, *, cookie: int, generation: int, last_sequence: int):
        if (type(cookie) is not int or not 0 < cookie < 2**64 or
            type(generation) is not int or not 0 < generation < 2**64 or
            type(last_sequence) is not int or not 0 <= last_sequence < 2**32):
            raise ValueError('Invalid publication cleanup identity')
        self.transport = transport
        self.cookie, self.generation, self.sequence = cookie, generation, last_sequence
        self.barrier_confirmed = self.closed = False
        self.native_verified = False  # Never satisfies ordinary uninstallation.
        self.outcome = None

    def _retained(self, receipt: CleanupReceipt | None) -> bool:
        return (isinstance(receipt, CleanupReceipt) and
                (receipt.magic, receipt.version, receipt.size, receipt.sequence,
                 receipt.owner_cookie, receipt.generation, receipt.status,
                 receipt.active, receipt.entry_restored, receipt.in_flight) ==
                (0x4B435254, 1, 48, self.sequence, self.cookie, self.generation,
                 RETAINED_BARRIER, 0, 0, 0))

    def stop(self) -> None:
        if self.closed:
            return
        # Transport must check the held original process HANDLE, not find a PID.
        if self.transport.game_exited():
            self.transport.release()
            self.closed, self.outcome = True, 'verified-game-exit'
            return
        if not self.barrier_confirmed:
            if self.sequence == 2**32 - 1:
                raise CleanupUncertain('Sequence exhausted; retain until verified game exit')
            self.sequence += 1
            receipt = self.transport.disarm(self.sequence)
            if not self._retained(receipt):
                # Even an ordinary "entry restored" acknowledgment is wrong for
                # this experiment. It cannot authorize freeing live resources.
                raise CleanupUncertain('Publication guard retention unconfirmed; keep controller alive')
            self.barrier_confirmed = True
        self.outcome = 'barrier-retained-awaiting-game-exit'

