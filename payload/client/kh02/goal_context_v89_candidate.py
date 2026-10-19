"""Opt-in authenticated Connected-slot-data adapter; no game side effects."""
from .goal_session_v89_candidate import GoalSlotAdmission


class GoalContextMixin:
    def __init__(self,*args,**kwargs):
        self.goal_admission=GoalSlotAdmission(kwargs['expected_key'])
        super().__init__(*args,**kwargs)

    def invalidate_history(self):
        self.goal_admission.disconnect()
        super().invalidate_history()

    def on_package(self,cmd,args):
        super().on_package(cmd,args)
        if cmd!='Connected' or self.history_fault is not None:return
        try:
            from .received import DeliveryKey
            self.goal_admission.connected(self.server,
                DeliveryKey(self.seed_name,args['team'],args['slot']),args['slot_data'])
        except (ValueError,TypeError,KeyError) as error:
            self.history_fault=str(error);self.invalidate_history()
            self.observe(dict(event='fault',reason=str(error),delivery_enabled=False))


def context_type():
    """Delay CommonClient import until an explicitly enabled application needs it."""
    from .ap_observation_context_candidate import ObservationContext
    class GoalObservationContext(GoalContextMixin,ObservationContext):pass
    return GoalObservationContext
