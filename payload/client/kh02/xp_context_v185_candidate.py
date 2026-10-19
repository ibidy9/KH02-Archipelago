from .xp_slot_v185_candidate import XPAdmission, parse
from .mickey_context_v172_candidate import context_type as previous_context

def context_type():
    class XPContext(previous_context()):
        def __init__(self, *args, **kwargs):
            self.xp_admission = XPAdmission(kwargs['expected_key'])
            super().__init__(*args, **kwargs)

        def invalidate_history(self):
            self.xp_admission.disconnect()
            super().invalidate_history()

        def on_package(self, cmd, args):
            if cmd != 'Connected':
                return super().on_package(cmd, args)
            try:
                from .received import DeliveryKey
                original = args['slot_data']
                parse(original)
                self.xp_admission.connected(self.server,
                    DeliveryKey(self.seed_name, args['team'], args['slot']), original)
                base = dict(original)
                base.pop('xp_multiplier_halves')
                base['schema_version'] = 3
                super().on_package(cmd, dict(args, slot_data=base))
            except (ValueError, TypeError, KeyError) as error:
                self.history_fault = str(error)
                self.invalidate_history()
                self.observe(dict(event='fault', reason=str(error), delivery_enabled=False))
    return XPContext
