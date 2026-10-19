"""Additive controlled difficulty request. Applied only on a later known load."""
import ctypes as c
import asyncio
from .production_transport import WindowsProductionTransport
from .production_policy_command import ProductionControlledBlock

class DifficultyRequest(c.Structure):
    _fields_=[('magic',c.c_uint32),('version',c.c_uint32),('size',c.c_uint32),('desired',c.c_uint32),
        ('sequence',c.c_uint64),('cookie',c.c_uint64),('challenge',c.c_uint64),('authority',c.c_uint64),('epoch',c.c_uint64),('scope',c.c_ubyte*32)]

class DifficultyPending(c.Structure):
    _fields_=[('cookie',c.c_uint64),('challenge',c.c_uint64),('scope',c.c_ubyte*32),
        ('sequence',c.c_uint64),('authority',c.c_uint64),('requested_epoch',c.c_uint64),('applied_epoch',c.c_uint64),
        ('desired',c.c_uint32),('phase',c.c_uint32),('result',c.c_uint32),('reserved',c.c_uint32)]

class DifficultyBlock(ProductionControlledBlock):
    _fields_=[('difficulty_request',DifficultyRequest),('difficulty_state',DifficultyPending),('difficulty_observed',c.c_uint64)]

class WindowsDifficultyTransport(WindowsProductionTransport):
    block_type=DifficultyBlock
    allowed_commands=(1,2,3,4,5,6,7)

    async def queue_difficulty(self,desired,epoch):
        if type(desired) is not int or not 0<=desired<=3 or type(epoch) is not int or not 0<epoch<2**64:
            raise ValueError('Explicit difficulty and owner epoch required')
        async with self._async_lock:
            try:
                return await self._queue_difficulty(desired,epoch)
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise

    async def query_difficulty(self):
        """Refresh current fields; historical phase3 is not current proof."""
        async with self._async_lock:
            try:
                if not await self._sync():raise RuntimeError('Current AP authority required')
                self.block.requested_authority=self._token
                reply=await self._offload(self._send,3)
                if reply.status!=1 or reply.active!=1:raise RuntimeError('Difficulty observation refused')
                return self.difficulty_status()
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise

    async def _queue_difficulty(self,desired,epoch):
        if not await self._sync():raise RuntimeError("Current AP authority required")
        challenge,scope=self.policy_binding_values()
        seq=self.block.difficulty_state.sequence+1
        if seq>=2**64:raise ValueError('Difficulty sequence exhausted')
        r=DifficultyRequest(0x4b484446,1,c.sizeof(DifficultyRequest),desired,seq,
            self.prepared.cookie,challenge,self._token,epoch,(c.c_ubyte*32).from_buffer_copy(scope))
        c.memmove(c.addressof(self.block.difficulty_request),c.addressof(r),c.sizeof(r))
        self.block.requested_authority=self._token
        reply=await self._offload(self._send,7)
        if reply.status!=1:raise RuntimeError('Difficulty request refused')
        state=self.difficulty_status()
        if state['sequence']!=seq or state['phase']!=1 or state['desired']!=desired:
            raise RuntimeError('Difficulty queue acknowledgement differs')
        return state

    def difficulty_status(self):
        s=self.block.difficulty_state
        return {name:int(getattr(s,name)) for name in ('sequence','requested_epoch','applied_epoch','desired','phase','result')} | {'observed':int(self.block.difficulty_observed)}
