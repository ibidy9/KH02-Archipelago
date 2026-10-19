"""Revoke native delivery before authenticated history/connection transitions."""
from .ap_observation_context_candidate import ObservationContext

class ControlledObservationContext(ObservationContext):
    def __init__(self,*args,**kwargs):
        self.native_revoke=lambda:None
        super().__init__(*args,**kwargs)
    def invalidate_history(self):
        self.native_revoke()
        super().invalidate_history()
    def on_package(self,cmd,args):
        if cmd in ('RoomInfo','Connected','ReceivedItems'):self.native_revoke()
        super().on_package(cmd,args)
