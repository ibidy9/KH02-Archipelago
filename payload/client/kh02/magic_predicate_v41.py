"""Development-only observation and gated magic-predicate experiment."""
import asyncio,ctypes as c
from .magic_observation_v38 import MagicBlock,WindowsMagicObservationTransport
from .difficulty_transport_candidate import DifficultyRequest

class MagicPredicate(c.Structure):
    _fields_=[('raw',c.c_uint64),('reasons',c.c_uint64*10),('accepted',c.c_uint64*7),('blocked',c.c_uint64*7)]+[(n,c.c_uint64) for n in ('epoch','observed','mask','enabled','sequence','last_receiver','last_actor','last_caller','last_command','last_grand')]

class MagicPredicateBlock(MagicBlock):
    _fields_=[('magic_predicate',MagicPredicate)]

class WindowsMagicPredicateTransport(WindowsMagicObservationTransport):
    block_type=MagicPredicateBlock
    allowed_commands=(1,2,3,4,5,6,7,8)
    def magic_predicate_status(self):
        p=self.block.magic_predicate
        return {n:list(getattr(p,n)) if n in ('reasons','accepted','blocked') else int(getattr(p,n)) for n,_ in p._fields_}

    async def set_magic_mask(self,mask,epoch):
        if type(mask) is not int or not 0<=mask<=127:raise ValueError('Seven-bit magic mask required')
        async with self._async_lock:
            try:
                if not await self._sync():raise RuntimeError('Current authority required')
                challenge,scope=self.policy_binding_values();seq=self.block.magic_predicate.sequence+1
                req=DifficultyRequest(0x4b484d47,1,c.sizeof(DifficultyRequest),mask,seq,
                    self.prepared.cookie,challenge,self._token,epoch,(c.c_ubyte*32).from_buffer_copy(scope))
                c.memmove(c.addressof(self.block.action_request),c.addressof(req),c.sizeof(req))
                self.block.requested_authority=self._token
                reply=await self._offload(self._send,8);state=self.magic_predicate_status()
                if reply.status!=1 or (state['sequence'],state['epoch'],state['mask'])!=(seq,epoch,mask):raise RuntimeError('Magic mask not acknowledged')
                return state
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise
