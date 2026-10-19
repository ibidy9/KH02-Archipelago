"""Discard an acknowledged query snapshot overlapping an active native hook.

Compatibility candidate, not yet live proven. No timeout retry, grant replay,
or cleanup relaxation. Command 3 also services pending native reconciliation;
its acknowledgement is not cancellation and that request must not be resent.
The v183 Mickey control preserves pending identity across this deferred query.
"""
from dataclasses import replace
from .mickey_transport_v172_candidate import MickeyTransport as Previous
from .route_transport_candidate import MAGIC


class MickeyTransport(Previous):
    def _send(self, command):
        if command != 3:
            return super()._send(command)
        if self.pending_sequence is not None:
            raise RuntimeError('Previous native response is uncertain')
        sequence = self.last_published + 1
        self.cleanup.sequence = sequence
        reply = self._request(command, sequence)
        if reply is None or (
            reply.magic, reply.version, reply.size, reply.sequence,
            reply.owner_cookie, reply.generation, reply.entry_restored
        ) != (MAGIC, 1, 48, sequence, self.prepared.cookie, 1, 0):
            raise RuntimeError('Native acknowledgement missing or mismatched; do not retry')
        if reply.in_flight:
            if reply.in_flight < 0 or reply.status not in (1, 3) or reply.active != 1:
                raise RuntimeError('Native acknowledgement missing or mismatched; do not retry')
            # Do not decode, publish, or act on this query's receipt snapshot.
            # Existing probe status3 means no admitted snapshot this tick.
            # Keep the real flight count and original acknowledgement intact.
            self.overlap_queries = getattr(self, 'overlap_queries', 0) + 1
            self.last_overlap_reply = reply
            return replace(reply, status=3)
        return reply
