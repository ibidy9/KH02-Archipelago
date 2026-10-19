"""Candidate command9: immutable goal configuration, native history projection.

Caller must obtain configuration from authenticated seed slot data. This module
does not register a new generation schema or install a travel gate.
"""
import asyncio
import ctypes as c
from .meter_access_v48 import MeterAccessBlock,WindowsMeterAccessTransport
from .difficulty_transport_candidate import DifficultyRequest
from .goal_access_candidate import Goal,TideRequirement

class GoalState(c.Structure):
    _fields_=[('sequence',c.c_uint64),('epoch',c.c_uint64)]+[(n,c.c_uint32) for n in
        ('configured','goal','requirement','start','schema','access','available','reserved')]

class GoalConfigBlock(MeterAccessBlock):
    _fields_=[('goal_request',DifficultyRequest),('goal_state',GoalState)]

class WindowsGoalConfigTransport(WindowsMeterAccessTransport):
    block_type=GoalConfigBlock
    allowed_commands=WindowsMeterAccessTransport.allowed_commands+(9,)

    async def configure_goal(self,configuration,epoch):
        configuration.validate()
        if type(epoch) is not int or not 0<epoch<2**64:raise ValueError('Admitted epoch required')
        goal=1 if configuration.selected_goal is Goal.BOSS_RUSH else 2
        requirement=(TideRequirement.WAYFINDERS,TideRequirement.ZODIACS,TideRequirement.WARDROBE).index(configuration.requirement)+1
        start=('Castle Town','The World Within','Forest of Thorns','Depths of Darkness').index(configuration.starting_world)
        desired=goal|(requirement<<8)|(start<<16)|(configuration.item_schema<<24)
        async with self._async_lock:
            try:
                if not await self._sync():raise RuntimeError('Current AP authority required')
                challenge,scope=self.policy_binding_values()
                sequence=int(self.block.goal_state.sequence)+1
                if sequence>=2**64:raise RuntimeError('Goal command sequence exhausted')
                request=DifficultyRequest(0x4b484743,1,c.sizeof(DifficultyRequest),desired,
                    sequence,self.prepared.cookie,challenge,self._token,epoch,
                    (c.c_ubyte*32).from_buffer_copy(scope))
                c.memmove(c.addressof(self.block.goal_request),c.addressof(request),c.sizeof(request))
                self.block.requested_authority=self._token
                reply=await self._offload(self._send,9)
                state=self.block.goal_state
                if reply.status!=1 or (state.sequence,state.configured,state.goal,state.requirement,state.start,state.schema)!=(sequence,1,goal,requirement,start,5):
                    raise RuntimeError('Goal configuration refused or acknowledgement differs')
                return {n:int(getattr(state,n)) for n,_ in state._fields_}
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise
