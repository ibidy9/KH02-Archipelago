"""Static audited chest identities; native observation supplies opened bits."""
from dataclasses import dataclass
from .objective_checks_v116_candidate import ObjectiveReporter
CHEST_IDS = {0: 131585, 7: 131586, 6: 131587, 9: 131588, 10: 131589, 1: 131590, 11: 131591, 12: 131592, 16: 131593, 36: 131594, 14: 131595, 18: 131596, 19: 131597, 2: 131598, 20: 131599, 21: 131600, 22: 131601, 23: 131602, 24: 131603, 25: 131604, 26: 131605, 28: 131606, 27: 131607, 3: 131608, 29: 131609, 30: 131610, 31: 131611, 32: 131612, 4: 131613, 39: 131614, 8: 131615, 5: 131616, 34: 131617, 13: 131618, 35: 131619, 15: 131620, 17: 131621, 40: 131622, 37: 131623, 38: 131624, 33: 131625}

@dataclass(frozen=True)
class ChestObservation:
    cookie:int
    epoch:int
    load_id:int
    owner:int
    suppressed:bool
    flags:bytes
    def completed(self):
        if (any(type(v) is not int or not 0<v<2**64 for v in (self.cookie,self.epoch,self.load_id,self.owner))
                or self.suppressed is not True or type(self.flags) is not bytes or len(self.flags)!=16):
            raise ValueError('Invalid admitted chest observation')
        bits=int.from_bytes(self.flags,'little')
        return frozenset(location for index,location in CHEST_IDS.items() if bits & (1<<index))

class CombinedChecks:
    def __init__(self,owner):
        ctx=owner.session.context
        args=dict(key=owner.prepared.storage.key,cookie=owner.prepared.cookie,current=owner.objective_report_current)
        self.objectives=ObjectiveReporter(ctx,read=owner.read_objectives,**args)
        self.chests=ObjectiveReporter(ctx,read=owner.read_chests,**args)
        self.context=ctx
    async def poll(self):
        snapshot=self.context.current_snapshot()
        result=await self.objectives.poll()
        if not self.context.snapshot_is_current(snapshot):return result
        return result+await self.chests.poll()
