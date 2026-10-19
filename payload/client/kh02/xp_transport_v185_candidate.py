"""Immutable authenticated XP setup using the existing command envelope."""
import ctypes as c
from .query_overlap_v184_candidate import MickeyTransport
from .difficulty_transport_candidate import DifficultyRequest

class XPTransport(MickeyTransport):
    allowed_commands = MickeyTransport.allowed_commands + (13,)

    async def configure_xp(self):
        config, snapshot = self.session.context.xp_admission.current(self.session.context)
        async with self._async_lock:
            try:
                if not await self._sync() or not self.session.context.snapshot_is_current(snapshot):
                    raise RuntimeError('Current AP history required for XP configuration')
                challenge, scope = self.policy_binding_values()
                request = DifficultyRequest(0x4b485850, 1, c.sizeof(DifficultyRequest),
                    config.halves, 1, self.prepared.cookie, challenge, self._token, 0,
                    (c.c_ubyte*32).from_buffer_copy(scope))
                self.block.proposal_size = c.sizeof(request)
                c.memmove(c.addressof(self.block.proposal), c.addressof(request), c.sizeof(request))
                self.block.requested_authority = self._token
                reply = await self._offload(self._send, 13)
                if reply.status != 1:
                    raise RuntimeError('XP startup configuration refused')
                after, current = self.session.context.xp_admission.current(self.session.context)
                if after != config or current != snapshot:
                    raise RuntimeError('XP admission changed during configuration')
                self.xp_configuration = config
                return config
            except BaseException:
                self.revoke()
                self.delivery_fault = True
                raise
