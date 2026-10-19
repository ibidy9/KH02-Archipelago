import ctypes as c
import time
from pathlib import Path
from .route_transport_candidate import WindowsRouteTransport,RoutePolicy,NativeReceipt,MAGIC,VERSION
from .controller_cleanup_candidate import CleanupReceipt
from .pipeline_exit_cleanup_candidate import PipelineExitCleanup
from .disk_observation_transport_candidate import checked_inputs
from .save_transport_candidate import SaveTransportBlock
from .save_evidence_candidate import parse_closed
from .received import DeliveryKey
class WindowsSaveTransport(WindowsRouteTransport):
    block_type=SaveTransportBlock
    message=0x8271
    mapping_prefix='KH02APSaveTransport'
    enabled_export='Kh02SaveTransportLiveEnabled'
    validate_export='Kh02ValidateSaveTransport'
    hook_export='Kh02SaveTransportHook'
    allowed_commands=(1,2,3)

    def __init__(self,dll,*,dll_sha256,preparation,preparation_sha256,pid,creation_time,window,timeout=2.0):
        value,payload=checked_inputs(preparation,preparation_sha256,dll=dll,dll_hash=dll_sha256,
            pid=pid,creation_time=creation_time,window=window)
        self.payload=payload;self.value=value;self.evidence=None
        super().__init__(Path(dll),dll_sha256=dll_sha256,pid=pid,creation_time=creation_time,window=window,
            policy=RoutePolicy(owner_cookie=value['owner_cookie'],generation=1),timeout=timeout)

    def start(self):
        try:super().start()
        finally:
            if self.arm_sent:
                self.cleanup=PipelineExitCleanup(self,cookie=self.policy.owner_cookie,generation=1,last_sequence=self.last_published)

    def _request(self,command,sequence):
        if command==1:
            if self.last_published or self.pending_sequence is not None:raise RuntimeError('ARM already sent')
            self.block.setup_size=len(self.payload);c.memmove(c.addressof(self.block.setup),self.payload,len(self.payload))
            path=self.value['runtime_journal'].encode('utf-8')
            if not 4<=len(path)<=1023:raise ValueError('Invalid journal path')
            self.block.path_size=len(path);c.memmove(c.addressof(self.block.path),path,len(path))
        return self._serialized_request(command,sequence)

    def attach_current(self,epoch):
        if self.pending_sequence is not None:raise RuntimeError('Previous command not acknowledged')
        sequence=self.last_published+1;request=self.block.request
        prefix=('combined-'+format(self.policy.owner_cookie,'016x')+'-'+str(epoch)).encode()
        request.command=4;request.sequence=sequence;request.epoch=epoch;request.raw_size=0;request.prefix_size=len(prefix)
        c.memmove(c.addressof(request.prefix),prefix,len(prefix))
        # Cleanup must advance beyond every possibly published request, even on timeout.
        self.cleanup.sequence=sequence
        reply=self._request(3,sequence)
        if reply is None or (reply.magic,reply.version,reply.size,reply.sequence,reply.owner_cookie,reply.generation,reply.status,reply.active,reply.entry_restored,reply.in_flight)!=(MAGIC,1,48,sequence,self.policy.owner_cookie,1,1,1,0,0) or request.response_sequence!=sequence or request.status!=1:
            raise RuntimeError('Native current-branch preparation not confirmed')

    def metadata(self):
        if self.block is None:return []
        b=self.block
        if (b.magic,b.version,b.size)!=(MAGIC,VERSION,c.sizeof(self.block_type)) or b.overflow or not 0<=b.count<=32:
            raise ValueError('Invalid or overflowing evidence metadata')
        # Published metadata only: never copy receipt/frozen payloads while alive.
        rows=[]
        for e in b.events[:b.count]:
            if e.complete==1:
                rows.append(dict(kind=e.kind,epoch=e.epoch,known=e.known,argument=e.argument,result=e.result,flags=e.flags,
                                 started=e.started,finished=e.finished,thread=e.thread,receipt_size=e.receipt_size))
        return rows

    def publications_metadata(self):
        b=self.block
        if b is None:return []
        if not 0<=b.publication_count<=4:raise ValueError('Invalid publication count')
        return [dict(complete=p.complete,result=p.result,started=p.started,finished=p.finished,thread=p.thread) for p in b.publications[:b.publication_count]]

    def release(self):
        if self.arm_sent and not self.game_exited():raise RuntimeError('Retain until original process exit')
        if self.block is not None:
            try:
                b=SaveTransportBlock.from_buffer_copy(self.block)
                if (b.magic,b.version,b.size)!=(MAGIC,VERSION,c.sizeof(b)):raise ValueError('Save mapping ABI mismatch')
                self.evidence=parse_closed(b.events,b.count,b.overflow,b.publications,b.publication_count,
                    key=DeliveryKey(self.value['seed'],self.value['team'],self.value['slot']),cookie=self.value['owner_cookie'],history=(),supported=(0x020209,))
            except ValueError as error:self.evidence=dict(complete=False,authorizes_delivery=False,error=str(error))
        super().release()

    def _serialized_request(self, command: int, sequence: int) -> CleanupReceipt | None:
        if command not in self.allowed_commands or not self.last_published < sequence < 2**32:
            raise ValueError("Invalid or reused native request sequence")
        if self.pending_sequence is not None:
            # A timeout is not a cancellation of the game-thread callback. Never
            # overwrite command fields while the previous request can still read them.
            if self.block.response_sequence != self.pending_sequence:
                return None
            self.pending_sequence = None
        self.block.command = command
        self.block.status = 0
        self.block.request_sequence = sequence
        self.last_published = sequence
        self.pending_sequence = sequence
        if not self.user.PostThreadMessageW(self.thread, self.message, 0, 0):
            return None
        deadline = time.monotonic() + getattr(self, "_acknowledgement_timeout", lambda _: self.timeout)(command)
        while time.monotonic() < deadline:
            if self.game_exited():
                return None
            if self.block.response_sequence == sequence:
                # Copy once after acknowledgment; never expose a mutable view to callers.
                reply = NativeReceipt.from_buffer_copy(self.block.cleanup)
                self.pending_sequence = None
                return CleanupReceipt(**{name: getattr(reply, name) for name, _ in reply._fields_})
            time.sleep(0.01)
        return None
