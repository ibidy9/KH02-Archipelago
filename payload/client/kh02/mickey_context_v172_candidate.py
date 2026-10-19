"""Schema3 adapter with immutable authenticated Mickey and goal admissions."""
from .mickey_slot_v172_candidate import MickeyAdmission, parse
from .mickey_shared_client_v171 import context_type as previous_context

def context_type():
    class MickeyContext(previous_context()):
        def __init__(self,*args,**kwargs):
            self.mickey_admission=MickeyAdmission(kwargs['expected_key'])
            super().__init__(*args,**kwargs)

        def invalidate_history(self):
            self.mickey_admission.disconnect()
            super().invalidate_history()

        def on_package(self,cmd,args):
            if cmd!='Connected':
                return super().on_package(cmd,args)
            try:
                from .received import DeliveryKey
                original=args['slot_data']
                parse(original)
                self.mickey_admission.connected(self.server,
                    DeliveryKey(self.seed_name,args['team'],args['slot']),original)
                # Existing goal parser gets its exact, separately validated schema.
                base=dict(original);base.pop('mickey');base['schema_version']=2
                adapted=dict(args,slot_data=base)
                super().on_package(cmd,adapted)
            except (ValueError,TypeError,KeyError) as error:
                self.history_fault=str(error);self.invalidate_history()
                self.observe(dict(event='fault',reason=str(error),delivery_enabled=False))
    return MickeyContext
