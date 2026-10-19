"""Read-only diagnostic ABI, separate from the retained v31 transport."""
import ctypes as c
from .difficulty_transport_candidate import DifficultyBlock,WindowsDifficultyTransport

KINDS=('Aerial Recovery','Barrier','Counter Blast','Cartwheel')
REASONS=('accepted','wrong_thread','inactive','fault','no_owner','stale_epoch',
         'no_session_observation','other_kind','receiver_read','unexpected_table',
         'local_player_mismatch','unstable_identity')


class Counters(c.Structure):
    _fields_=[('raw',c.c_uint64),('reasons',c.c_uint64*12),('target_attempts',c.c_uint64*4)]


class Sample(c.Structure):
    _fields_=[(n,c.c_uint64) for n in ('receiver','vtable','actor','provider','caller')]+[
        ('kind',c.c_uint32),('reason',c.c_int32)]+[(n,c.c_uint64) for n in ('manager','action_state','state_vtable')]+[('state_kind',c.c_uint32),('substate',c.c_uint32)]


class Snapshot(c.Structure):
    _fields_=[('epoch',c.c_uint64),('overflow',c.c_uint64),('coherent',c.c_uint64),
              ('hooks',Counters*4),('epoch_counts',(c.c_uint64*4)*4),
              ('samples',(Sample*4)*4),('callers',((c.c_uint64*4)*4)*4)]


class AbilityDiagnosticBlock(DifficultyBlock):
    _fields_=[('default_abilities',Snapshot)]


def decode(value):
    result={'epoch':int(value.epoch),'overflow':int(value.overflow),
            'coherent':bool(value.coherent),'hooks':{}}
    for h,name in enumerate(('recovery_predicate','counter_and_other_queries','guard_predicate','dodge_predicate')):
        counters=value.hooks[h]
        reasons=dict(zip(REASONS,map(int,counters.reasons)))
        result['hooks'][name]={'raw':int(counters.raw),'reasons':reasons,
            'accounted':sum(reasons.values())==counters.raw,'abilities':{}}
        for i,kind in enumerate(KINDS):
            sample=value.samples[h][i]
            result['hooks'][name]['abilities'][kind]={
                'attempts':int(counters.target_attempts[i]),
                'epoch_accepted':int(value.epoch_counts[h][i]),
                'sample':{n:int(getattr(sample,n)) for n,_ in Sample._fields_},
                'caller_rvas':[int(x) for x in value.callers[h][i] if x]}
    return result


class WindowsAbilityDiagnosticTransport(WindowsDifficultyTransport):
    block_type=AbilityDiagnosticBlock

    def default_ability_observation(self):
        if self.block is None:raise RuntimeError('Observer not attached')
        # Do not throw away diagnostic rejects or partial/overflow evidence.
        return decode(self.block.default_abilities)
