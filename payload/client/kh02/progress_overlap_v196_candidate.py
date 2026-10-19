"""Discard only acknowledged read-only objective, chest and progress snapshots on overlap.

No native replay, timeout recovery, or relaxation of mutations/other actions.
Existing read_progress returns None for status2 and does not decode its payload.
"""
import struct
from dataclasses import replace
from .shutdown_transport_v191_candidate import ShutdownTransport
from .route_transport_candidate import MAGIC


class ProgressTransport(ShutdownTransport):
    def _send(self, command):
        diagnostic = False
        if command == 11 and self.block.proposal_size == 40:
            magic, epoch, cookie, load, action, reserved = struct.unpack(
                '<4Q2I', bytes(self.block.proposal[:40]))
            diagnostic = (magic == 0x3631314f5041 and epoch > 0 and load > 0
                          and cookie == self.prepared.cookie and action in (2, 4, 11) and reserved == 0)
        if not diagnostic:
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
            if reply.in_flight < 0 or reply.status not in (1, 2) or reply.active != 1:
                raise RuntimeError('Native acknowledgement missing or mismatched; do not retry')
            self.progress_overlap_queries = getattr(self, 'progress_overlap_queries', 0) + 1
            self.last_progress_overlap_reply = reply
            return replace(reply, status=2)
        return reply
