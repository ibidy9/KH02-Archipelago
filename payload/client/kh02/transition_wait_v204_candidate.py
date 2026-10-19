"""Wait longer for the same status request across a game-thread transition.

Command 3 can reconcile rewards: it is never resent. Identity validation,
pending-request exclusion and cleanup deadlines remain inherited unchanged.
"""
from .progress_overlap_v196_candidate import ProgressTransport


class TransitionWaitTransport(ProgressTransport):
    def _acknowledgement_timeout(self, command: int) -> float:
        return 30.0 if command == 3 else self.timeout
