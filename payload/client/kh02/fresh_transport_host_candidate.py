"""Client adapter for the separate, effects-off fresh transport.

Uses the proven process/window checks and exit-only cleanup. No game launch,
save deletion, item delivery or resealing after launch is implemented here.
"""
import ctypes as c
from dataclasses import dataclass
from pathlib import Path
from .fresh_setup_candidate import encode_setup,setup_nonce,MAX_SIZE
from .fresh_transport_candidate import FreshTransportBlock,FreshEvent
from .route_transport_candidate import WindowsRouteTransport,RoutePolicy,MAGIC,VERSION
from .pipeline_exit_cleanup_candidate import PipelineExitCleanup
from .storage_session_candidate import StoragePreparation,scope_binding

@dataclass(frozen=True)
class FreshPreparation:
    storage: StoragePreparation
    payload: bytes
    nonce: bytes
    cookie: int
    history: tuple

def prepare_fresh(session,storage,*,cookie,supported,journal_path):
    """Call inside StorageSession's synchronous, prelaunch publish callback.

    The caller must put result.nonce in the ready intent and preserve result.
    Recheck snapshot validity when publishing files, as StorageSession requires.
    """
    snapshot=session.snapshot()
    if snapshot.key!=storage.key or storage.binding!=scope_binding(snapshot.key):
        raise ValueError('Fresh setup does not match the authenticated storage scope')
    history=snapshot.received_items()
    payload=encode_setup(key=storage.key,history=history,supported=tuple(supported),
        cookie=cookie,generation=storage.generation,journal_path=journal_path)
    if not session.context.snapshot_is_current(snapshot):raise ValueError('History changed during setup preparation')
    return FreshPreparation(storage,payload,setup_nonce(payload),cookie,history)

class WindowsFreshTransport(WindowsRouteTransport):
    block_type=FreshTransportBlock
    message=0x8271
    mapping_prefix='KH02APFreshTransport'
    enabled_export='Kh02FreshTransportLiveEnabled'
    validate_export='Kh02ValidateFreshTransport'
    hook_export='Kh02FreshTransportHook'

    def __init__(self,dll,*,dll_sha256,session,prepared,proof,window,timeout=2.0):
        if not isinstance(prepared,FreshPreparation) or type(prepared.payload) is not bytes or not 128<=len(prepared.payload)<=MAX_SIZE:
            raise ValueError('Immutable prepared fresh setup required')
        if setup_nonce(prepared.payload)!=prepared.nonce:raise ValueError('Prepared setup seal changed')
        self.session,self.prepared,self.proof=session,prepared,proof
        self.evidence=None
        self._check_session()
        super().__init__(Path(dll),dll_sha256=dll_sha256,pid=proof.pid,creation_time=proof.created,
            window=window,policy=RoutePolicy(owner_cookie=prepared.cookie,generation=1),timeout=timeout)

    def _check_session(self):
        if self.session.faulted or self.session.preparation!=self.prepared.storage or self.session.process!=self.proof:
            raise ValueError('Fresh transport requires the admitted startup process')
        snapshot=self.session.snapshot()
        history=snapshot.received_items()
        if snapshot.key!=self.prepared.storage.key or history[:len(self.prepared.history)]!=self.prepared.history:
            raise ValueError('Authenticated history no longer contains the prepared prefix')

    def start(self):
        self._check_session()
        try:super().start()
        finally:
            if self.arm_sent:
                self.cleanup=PipelineExitCleanup(self,cookie=self.policy.owner_cookie,
                    generation=1,last_sequence=self.last_published)

    def _request(self,command,sequence):
        if command==1:
            if self.last_published or self.pending_sequence is not None:raise RuntimeError('Fresh ARM already submitted')
            self._check_session()
            self.block.setup_size=len(self.prepared.payload)
            c.memmove(c.addressof(self.block.setup),self.prepared.payload,len(self.prepared.payload))
        return super()._request(command,sequence)

    def release(self):
        if self.arm_sent and not self.game_exited():raise RuntimeError('Retain fresh transport until original process exit')
        if self.block is not None:
            b=self.block_type.from_buffer_copy(self.block)
            capacity=len(b.events)
            complete=(b.magic,b.version,b.size)==(MAGIC,VERSION,c.sizeof(self.block_type)) and \
                b.evidence_complete==1 and b.evidence_sequence>0 and not b.overflow and 0<=b.count<=capacity and \
                (b.cleanup.sequence,b.cleanup.owner_cookie,b.cleanup.status,b.cleanup.active,b.cleanup.in_flight)== \
                (b.evidence_sequence,self.policy.owner_cookie,8,0,0)
            events=[{name:getattr(e,name) for name,_ in FreshEvent._fields_} for e in b.events[:min(b.count,capacity)]]
            self.evidence=dict(complete=complete and all(e['complete']==1 for e in events),
                sequence=b.evidence_sequence,count=b.count,overflow=b.overflow,events=events,authorizes_delivery=False)
        super().release()
