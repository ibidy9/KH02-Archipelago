"""Additive companion transition handling; existing shared services unchanged."""
from .mickey_shared_client_v171 import SharedClient as Core, ServiceStatus
from .mickey_shared_client_v172_candidate import SharedClient as Previous
from .mickey_control_v183_candidate import MickeyControl


class SharedClient(Previous):
    def __init__(self, owner, **kwargs):
        super().__init__(owner, **kwargs)
        if self.mickey_control is not None:
            self.mickey_control.close()
            self.mickey_control = None
            try:
                self.mickey_control = MickeyControl(owner)
            except BaseException:
                self.close()
                raise

    async def _mickey_tick(self):
        status = await Core.tick(self)
        if status.state != 'active' or self.mickey_control is None:
            return status
        try:
            config, _ = self.context.mickey_admission.current(self.context)
            if config != self.mickey_config:
                raise ValueError('Mickey configuration changed')
            reply = await self.mickey_control.request(2)
            if reply.get('deferred') and reply['status'] == 3 and reply['result'] == 40:
                return ServiceStatus('waiting_for_owner', goal=status.goal, delivery=status.delivery)
            if reply['status'] != 1:
                raise RuntimeError('Mickey reconciliation refused: ' + str(reply))
            return status
        except BaseException:
            self.owner.revoke()
            self.faulted = True
            raise
