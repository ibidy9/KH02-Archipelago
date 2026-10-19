"""Development action filter controls; AP item integration is a later step."""
import asyncio
import ctypes as c
from .default_gates_observer_v36 import AbilityDiagnosticBlock,WindowsAbilityDiagnosticTransport
from .difficulty_transport_candidate import DifficultyRequest

class ActionGateBlock(AbilityDiagnosticBlock):
    _fields_=[('action_request',DifficultyRequest)]+[(n,c.c_uint64) for n in (
        'action_sequence','action_epoch','action_mask','action_recovery_blocked','action_counter_blocked','action_guard_blocked','action_dodge_blocked')]

class WindowsActionGateTransport(WindowsAbilityDiagnosticTransport):
    block_type=ActionGateBlock
    allowed_commands=(1,2,3,4,5,6,7,8)

    def action_gate_status(self):
        return {n:int(getattr(self.block,'action_'+n)) for n in
                ('sequence','epoch','mask','recovery_blocked','counter_blocked','guard_blocked','dodge_blocked')}

    async def set_action_gates(self,mask,epoch):
        if type(mask) is not int or not 0<=mask<=15 or type(epoch) is not int or not 0<epoch<2**64:
            raise ValueError('Explicit four-bit unlock mask and current epoch required')
        async with self._async_lock:
            try:
                if not await self._sync():raise RuntimeError('Current AP authority required')
                challenge,scope=self.policy_binding_values()
                seq=self.block.action_sequence+1
                if seq>=2**64:raise RuntimeError('Action sequence exhausted')
                req=DifficultyRequest(0x4b484146,1,c.sizeof(DifficultyRequest),mask,seq,
                    self.prepared.cookie,challenge,self._token,epoch,
                    (c.c_ubyte*32).from_buffer_copy(scope))
                c.memmove(c.addressof(self.block.action_request),c.addressof(req),c.sizeof(req))
                self.block.requested_authority=self._token
                reply=await self._offload(self._send,8)
                state=self.action_gate_status()
                if reply.status!=1 or (state['sequence'],state['epoch'],state['mask'])!=(seq,epoch,mask):
                    raise RuntimeError('Action gate request not acknowledged; do not retry')
                return state
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise
