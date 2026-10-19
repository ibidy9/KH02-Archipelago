"""Immutable Mickey startup configuration through the existing owner channel."""
import asyncio
import ctypes as c
from .rush_transport_v162_candidate import RushTransport as ObjectiveTransport
from .difficulty_transport_candidate import DifficultyRequest

class MickeyTransport(ObjectiveTransport):
    allowed_commands=ObjectiveTransport.allowed_commands+(12,)

    async def configure_mickey(self):
        config,snapshot=self.session.context.mickey_admission.current(self.session.context)
        async with self._async_lock:
            try:
                if not await self._sync():
                    raise RuntimeError('Current AP history required for Mickey configuration')
                if not self.session.context.snapshot_is_current(snapshot):
                    raise RuntimeError('AP history changed during Mickey configuration')
                challenge,scope=self.policy_binding_values()
                request=DifficultyRequest(0x4b484d43,1,c.sizeof(DifficultyRequest),
                    {'vanilla':0,'randomized_depths':1,'randomized_all_worlds':2}[config.mode],1,self.prepared.cookie,
                    challenge,self._token,0,(c.c_ubyte*32).from_buffer_copy(scope))
                self.block.proposal_size=c.sizeof(request)
                c.memmove(c.addressof(self.block.proposal),c.addressof(request),c.sizeof(request))
                self.block.requested_authority=self._token
                reply=await self._offload(self._send,12)
                if reply.status!=1:
                    raise RuntimeError('Mickey startup configuration refused')
                after,current=self.session.context.mickey_admission.current(self.session.context)
                if after!=config or not self.session.context.snapshot_is_current(snapshot):
                    raise RuntimeError('Mickey admission changed during configuration')
                self.mickey_configuration=config
                return config
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise
