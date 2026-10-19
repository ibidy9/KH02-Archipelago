"""Transient DeathLink, scoped to one authenticated connection and living owner.

No AP item, save membership, persistent death queue or independent memory writer.
"""
import ctypes as c
import math
import struct
import time
from collections import deque

MAGIC=0x393432444c5041
FIELDS=('magic','id','epoch','kind','status','reason','calls','hp','mp','state',
        'paused','context_flags','actor','last_id','native_result','reserved')
DEAD_STATES=(0x33,0x5e)

def normalize_slot(slot):
    value=dict(slot)
    if value.get('schema_version') in (4,5):
        if 'death_link' in value:raise ValueError('DeathLink requires slot schema6')
        return value,False
    if value.get('schema_version')!=6 or type(value.get('schema_version')) is not int:
        raise ValueError('Unsupported DeathLink slot schema')
    enabled=value.pop('death_link',None)
    if type(enabled) is not bool:raise ValueError('DeathLink must be boolean')
    value['schema_version']=5
    return value,enabled

async def request(owner,kind=0,identity=0):
    owner.allowed_commands=tuple(sorted(set(owner.allowed_commands+(15,))))
    async with owner._async_lock:
        if not await owner._sync():raise RuntimeError('DeathLink lacks current AP authority')
        epoch=owner.block.meter_access.epoch
        packet=struct.pack('<QQQII',MAGIC,identity,epoch,kind,0)
        owner.block.requested_authority=owner._token;owner.block.proposal_size=len(packet)
        c.memmove(c.addressof(owner.block.proposal),packet,len(packet))
        if kind==0:
            from .death_query_v251 import send_query
            reply=await owner._offload(send_query,owner)
            if reply is None:return dict(status=3,deferred=True)
        else:
            reply=await owner._offload(owner._send,15)
        if owner.block.proposal_size!=128:raise RuntimeError('DeathLink acknowledgement missing; do not retry')
        v=dict(zip(FIELDS,struct.unpack('<16Q',bytes(owner.block.proposal)[:128])))
        if (v['magic'],v['id'],v['epoch'],v['status'])!=(MAGIC,identity,epoch,reply.status):
            raise RuntimeError('DeathLink acknowledgement mismatch; do not retry')
        return v

class LinkState:
    def __init__(self):
        self.enabled=False;self.eligible=None;self.pending=None
        self.seen=deque(maxlen=256);self.serial=time.time_ns()

    def clear(self):
        self.eligible=None;self.pending=None

    def receive(self,data,connection):
        if not self.enabled or not self.eligible or self.eligible[0] is not connection:
            return False
        if not isinstance(data,dict):return False
        stamp=data.get('time');source=data.get('source');cause=data.get('cause','')
        if type(stamp) not in (int,float) or not math.isfinite(stamp) or stamp<=0:
            return False
        if type(source) is not str or not source or len(source)>256 or type(cause) is not str or len(cause)>4096:
            return False
        key=(stamp,source)
        if key in self.seen:return False
        self.seen.append(key)
        # Multiple deaths before one application collapse to one, not a death chain.
        if self.pending is None:
            self.serial+=1
            self.pending=(self.eligible,self.serial,source)
        return True

class DeathLinkService:
    def __init__(self,owner,context,*,send=request,clock=time.monotonic):
        self.owner=owner;self.context=context;self.link=context.death_link_state
        self.send=send;self.clock=clock;self.identity=None;self.tag_connection=None
        self.armed=False;self.alive_since=None;self.remote=False;self.latest=None;self.saw_dead=False
        self.observation=None
        if owner is not None:
            from .death_observation_v250 import DeathObservation
            self.observation=DeathObservation(owner)

    def close(self):
        self.defer()
        if self.observation is not None:self.observation.close()

    def defer(self):
        self.link.clear();self.identity=None;self.armed=False;self.alive_since=None;self.remote=False;self.saw_dead=False

    async def tick(self,*,active,can_apply):
        ctx=self.context
        if not self.link.enabled:
            self.defer();return
        try:snapshot=ctx.current_snapshot()
        except (ValueError,RuntimeError):self.defer();return
        if self.tag_connection is not snapshot.connection:
            self.defer();self.tag_connection=snapshot.connection
            await ctx.update_death_link(True)
        # Read the retained native event BEFORE inactive gameplay clears identity.
        row=self.observation.read() if self.observation is not None else None
        if row is not None:
            match=self.identity==(snapshot.connection,row['epoch'],row['actor'])
            if (match and self.armed and not self.remote and not row['remote']
                    and row['hp']==0 and row['result']==1 and ctx.snapshot_is_current(snapshot)):
                self.armed=False;self.saw_dead=True;self.alive_since=None;self.link.clear()
                name=ctx.player_names.get(ctx.slot,'Aqua')
                await ctx.send_death(f'{name} fell in Kingdom Hearts 0.2.')
                ctx.observe(dict(event='deathlink_sent',native_observation=row))
        if not active:self.defer();return
        v=await self.send(self.owner);self.latest=v
        if not ctx.snapshot_is_current(snapshot):self.defer();return
        if v.get('deferred'):
            self.link.eligible=None
            return
        if v['status']!=1:self.defer();return
        identity=(snapshot.connection,v['epoch'],v['actor'])
        if identity!=self.identity:
            self.defer();self.identity=identity
        dead=v['state'] in DEAD_STATES and v['hp']==0
        if dead:
            self.saw_dead=True
            report=self.armed and not self.remote
            self.armed=False;self.alive_since=None;self.link.clear()
            if report:
                # Latch before network await; never echo a received death.
                name=ctx.player_names.get(ctx.slot,'Aqua')
                await ctx.send_death(f'{name} fell in Kingdom Hearts 0.2.')
                ctx.observe(dict(event='deathlink_sent',epoch=v['epoch']))
            return
        if not v['hp'] or v['context_flags']&2 or v['state'] in DEAD_STATES:
            self.link.eligible=None
            return
        if self.saw_dead:
            self.remote=False;self.saw_dead=False;self.alive_since=None
        now=self.clock()
        if self.alive_since is None:self.alive_since=now
        if now-self.alive_since<2:return
        if not self.remote:
            self.armed=True
            self.link.eligible=identity
        # A native attempt remains suppressed until a confirmed death/recovery
        # or a changed owner. A failed kill must not cause repeated HP writes.
        if self.remote:return
        pending=self.link.pending
        if not pending or pending[0]!=identity:return
        if v['paused'] or not can_apply:return
        if not ctx.snapshot_is_current(snapshot):self.defer();return
        self.link.pending=None;self.link.eligible=None;self.remote=True;self.armed=False
        result=await self.send(self.owner,1,pending[1])
        attempted=result['last_id']==pending[1]
        if not attempted and result['status']==2 and result['reason'] in (1,2,3,5,6):
            self.remote=False;self.link.pending=pending
        ctx.observe(dict(event='deathlink_received',source=pending[2],attempted=attempted,
                         result=result))

    def recovered(self):
        """Reserved for a proven recovery boundary; normal owner changes reset it."""
        self.defer()
