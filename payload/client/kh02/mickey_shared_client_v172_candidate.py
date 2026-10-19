"""Automatic Depths reconciliation alongside the proven shared owner."""
import asyncio
from .mickey_shared_client_v171 import SharedClient as Previous
from .mickey_control_v171 import MickeyControl

class SharedClient(Previous):
    def __init__(self,owner,**kwargs):
        config,_=owner.session.context.mickey_admission.current(owner.session.context)
        if getattr(owner,'mickey_configuration',None)!=config:
            raise ValueError('Configure Mickey before loading gameplay')
        self.mickey_tick_lock=asyncio.Lock()
        self.mickey_config=config
        self.mickey_control=None
        super().__init__(owner,**kwargs)
        try:
            if config.mode in ('randomized_depths','randomized_all_worlds'):self.mickey_control=MickeyControl(owner)
        except BaseException:
            super().close()
            raise

    async def tick(self):
        async with self.mickey_tick_lock:
            return await self._mickey_tick()

    async def _mickey_tick(self):
        status=await super().tick()
        if status.state!='active' or self.mickey_control is None:return status
        try:
            config,_=self.context.mickey_admission.current(self.context)
            if config!=self.mickey_config:raise ValueError('Mickey configuration changed')
            reply=await self.mickey_control.request(2)
            if reply['status']!=1:raise RuntimeError('Mickey reconciliation refused: '+str(reply))
            return status
        except BaseException:
            self.owner.revoke();self.faulted=True;raise

    def close(self):
        try:
            if self.mickey_control is not None:
                self.mickey_control.close();self.mickey_control=None
        finally:super().close()
