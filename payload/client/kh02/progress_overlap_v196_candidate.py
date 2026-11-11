"""Discard only acknowledged read-only objective, chest and progress snapshots on overlap.

No native replay, timeout recovery, or relaxation of mutations/other actions.
Existing read_progress returns None for status2 and does not decode its payload.
"""

from dataclasses import replace
from .shutdown_transport_v191_candidate import ShutdownTransport, progress_query_action
from .route_transport_candidate import MAGIC


class ProgressTransport(ShutdownTransport):
    def _send(self, command):
        action = progress_query_action(self, command)
        if action is None:
            return super()._send(command)
        if self.pending_sequence is not None:
            raise RuntimeError("Previous native response is uncertain")
        sequence = self.last_published + 1
        self.cleanup.sequence = sequence
        reply = self._request(command, sequence)

        def record_failure():
            fields = (
                "magic",
                "version",
                "size",
                "sequence",
                "owner_cookie",
                "generation",
                "entry_restored",
                "in_flight",
                "status",
                "active",
            )
            self.query_diagnostic = dict(
                command=command,
                action=action,
                requested_sequence=sequence,
                expected_cookie=self.prepared.cookie,
                reply=None if reply is None else {name: getattr(reply, name) for name in fields},
            )

        if reply is None or (
            reply.magic,
            reply.version,
            reply.size,
            reply.sequence,
            reply.owner_cookie,
            reply.generation,
            reply.entry_restored,
        ) != (MAGIC, 1, 48, sequence, self.prepared.cookie, 1, 0):
            record_failure()
            raise RuntimeError("Native acknowledgement missing or mismatched; do not retry")
        if reply.in_flight:
            if reply.in_flight < 0 or reply.status not in (1, 2) or reply.active != 1:
                record_failure()
                raise RuntimeError("Native acknowledgement missing or mismatched; do not retry")
            self.progress_overlap_queries = getattr(self, "progress_overlap_queries", 0) + 1
            self.last_progress_overlap_reply = reply
            return replace(reply, status=2)
        return reply
