"""Read-only objective queries; native save lifecycle owns activation."""
import asyncio
import ctypes as c
import struct

from .chest_transport_v125_candidate import ChestTransport as NotificationTransport
from dataclasses import dataclass

@dataclass(frozen=True)
class RushObservation:
    cookie:int
    epoch:int
    load_id:int
    owner:int
    header:bytes
    switches:bytes
    exits:int
    reason:int
    hits:int
    darksides:int
    darkside_reason:int
    darkside_hits:int
    rush:int
    rush_reason:int
    rush_hits:int

REPLY = struct.Struct('<4Q2I64s8s2IQ2IQ2IQ')


def decode_observation(raw):
    if type(raw) is not bytes or len(raw) != REPLY.size:
        raise ValueError('Invalid objective response size')
    values = REPLY.unpack(raw)
    if values[4] != 1 or values[5]:
        raise ValueError('Invalid objective response fields')
    result = RushObservation(*values[:4], *values[6:])
    if any(not 0<v<2**64 for v in values[:4]):raise ValueError('Invalid progress binding')
    if result.exits & ~15 or result.reason>5:raise ValueError("Invalid exit fields")
    if result.darksides & ~7 or result.darkside_reason>4:raise ValueError("Invalid Darkside fields")
    if result.rush & ~31 or result.rush_reason>4:raise ValueError("Invalid rush fields")
    return result


class RushTransport(NotificationTransport):
    allowed_commands = NotificationTransport.allowed_commands

    async def read_progress(self):
        # probe() keeps its existing lock and all receipt/history validation.
        # The subsequent command repeats the exact cookie/load/epoch binding.
        observed = await self.probe()
        if observed is None:
            return None
        epoch, receipt = observed
        packet = struct.pack('<4Q2I', 0x3631314f5041, epoch,
                             receipt.owner_cookie, receipt.load_id, 10, 0)
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
