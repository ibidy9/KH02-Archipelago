"""Read-only objective queries; native save lifecycle owns activation."""
import asyncio
import ctypes as c
import struct

from .objective_transport_v119_candidate import ObjectiveTransport as NotificationTransport
from .objective_checks_v116_candidate import ObjectiveObservation

REPLY = struct.Struct('<4Q54I')


def decode_observation(raw):
    if type(raw) is not bytes or len(raw) != REPLY.size:
        raise ValueError('Invalid objective response size')
    values = REPLY.unpack(raw)
    if values[4] != 1 or values[5] or values[-1]:
        raise ValueError('Invalid objective response fields')
    result = ObjectiveObservation(*values[:4], True, tuple(values[6:-1]))
    result.completed()
    return result


class ObjectiveTransport(NotificationTransport):
    allowed_commands = NotificationTransport.allowed_commands

    async def read_objectives(self):
        # probe() keeps its existing lock and all receipt/history validation.
        # The subsequent command repeats the exact cookie/load/epoch binding.
        observed = await self.probe()
        if observed is None:
            return None
        epoch, receipt = observed
        packet = struct.pack('<4Q2I', 0x3631314f5041, epoch,
                             receipt.owner_cookie, receipt.load_id, 2, 0)
        async with self._async_lock:
            try:
                if not self.history.ready or not self.block.authority:
                    return None
                self._check_session()
                self.block.requested_authority = self._token
                self.block.proposal_size = len(packet)
                c.memmove(c.addressof(self.block.proposal), packet, len(packet))
                reply = await self._offload(self._send, 11)
                if reply.status == 2:
                    return None  # No admitted gameplay sample; safe to poll later.
                if reply.status != 1 or reply.active != 1:
                    raise RuntimeError('Objective suppression uncertain; stop this candidate')
                size = self.block.proposal_size
                if size != REPLY.size:
                    raise ValueError('Malformed native objective reply')
                observation = decode_observation(bytes(self.block.proposal[:size]))
                if (observation.cookie, observation.epoch, observation.load_id) != (
                        receipt.owner_cookie, epoch, receipt.load_id):
                    raise ValueError('Objective query binding changed')
                return observation, receipt, epoch
            except (Exception, asyncio.CancelledError):
                self.revoke()
                self.delivery_fault = True
                raise

    def objective_report_current(self, observation):
        # This guards the captured report's process/AP authority, not a claim
        # that a later game frame still has the same objective state. Completion
        # was observed synchronously in the admitted native command above.
        self._check_session()
        return (not self.game_exited() and self.history.ready and not self.delivery_fault
                and self.block.authority == self._token
                and self.block.acknowledged_authority == self._token
                and observation.cookie == self.prepared.cookie)
