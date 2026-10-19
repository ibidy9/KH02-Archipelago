"""Guarded mixed delivery ABI and client coordinator; launch remains manual."""
import asyncio
import ctypes as c
import threading
from dataclasses import asdict
from .fresh_transport_candidate import ReusableFreshTransportBlock
from .fresh_transport_host_candidate import WindowsFreshTransport
from .save_transport_host_candidate import WindowsSaveTransport
from .history_command_candidate import encode_history
from .coordinated_history_candidate import CoordinatedHistory
from .receipt_set_candidate import ReceiptSet,encode,decode
from .route_transport_candidate import MAGIC

class MixedOutcome(c.Structure):
    _fields_=[('epoch',c.c_uint64),('sequence',c.c_uint64)]+[(n,c.c_uint32) for n in ('status','kind','entered','before','after','reserved')]

class ControlledBlock(ReusableFreshTransportBlock):
    _fields_=[('authority',c.c_int64),('requested_authority',c.c_uint64),('acknowledged_authority',c.c_uint64),
        ('epoch',c.c_uint64),('attempt_sequence',c.c_uint64),('kind',c.c_uint32),('proposal_size',c.c_uint32),
        ('proposal',c.c_ubyte*102456),('outcome',MixedOutcome),('query_epoch',c.c_uint64),
        ('query_size',c.c_uint32),('query_reserved',c.c_uint32),('receipts',c.c_ubyte*102456)]

class WindowsControlledTransport(WindowsFreshTransport):
    block_type=ControlledBlock
    allowed_commands=(1,2,3,4,5)
    def __init__(self,*args,record,**kwargs):
        if not callable(record):raise ValueError('Durable attempt recorder required')
        self.record=record;self._release_lock=threading.RLock();self._async_lock=asyncio.Lock()
        self._token=0;self._attempt=0;self.delivery_fault=False;self.acknowledgments=[]
        super().__init__(*args,**kwargs)
        self.history=CoordinatedHistory(self.session.context,self.prepared.storage.key,self.prepared.history)

    def revoke(self):
        # Only this aligned authority field may change while a request is pending.
        with self._release_lock:
            if self.block is not None:self.block.authority=0

    def _request(self,command,sequence):
        if command==1:
            self._check_session()
            self.block.setup_size=len(self.prepared.payload)
            c.memmove(c.addressof(self.block.setup),self.prepared.payload,len(self.prepared.payload))
        return WindowsSaveTransport._serialized_request(self,command,sequence)

    def _send(self,command):
        if self.pending_sequence is not None:raise RuntimeError('Previous native response is uncertain')
        sequence=self.last_published+1;self.cleanup.sequence=sequence
        reply=self._request(command,sequence)
        if reply is None or (reply.magic,reply.version,reply.size,reply.sequence,reply.owner_cookie,
            reply.generation,reply.entry_restored,reply.in_flight)!=(MAGIC,1,48,sequence,self.prepared.cookie,1,0,0):
            raise RuntimeError('Native acknowledgement missing or mismatched; do not retry')
        return reply

    async def _offload(self,call,*args):return await asyncio.to_thread(call,*args)

    async def _sync(self):
        if self.delivery_fault:raise RuntimeError('Delivery is faulted')
        try:self.session.context.current_snapshot()
        except RuntimeError:
            self.revoke();return False
        if self.history.ready and self.block.authority and self.block.authority==self.block.acknowledged_authority:return True
        self.revoke();self._token+=1
        if self._token>=2**63:raise RuntimeError('Authority sequence exhausted')
        token=self._token
        async def apply(items):
            payload=encode_history(self.prepared.storage.key,self.prepared.cookie,1,items)
            self.block.setup_size=len(payload);c.memmove(c.addressof(self.block.setup),payload,len(payload))
            self.block.requested_authority=token;self.block.authority=token
            return (await self._offload(self._send,4)).status
        return await self.history.synchronize(apply)

    async def probe(self):
        async with self._async_lock:
            try:
                if not await self._sync():return None
                self.block.requested_authority=self._token
                reply=await self._offload(self._send,3)
                if reply.status==3:return None
                if reply.status!=1 or reply.active!=1:raise RuntimeError('Receipt query refused')
                if not self.history.ready:return None
                size=self.block.query_size;epoch=self.block.query_epoch
                if not epoch or not 56<=size<=102456 or self.block.query_reserved:raise RuntimeError('Invalid receipt query')
                receipt=decode(bytes(self.block.receipts[:size]))
                if receipt.key!=self.prepared.storage.key or receipt.owner_cookie!=self.prepared.cookie:raise RuntimeError('Foreign receipt query')
                current=self.history.native_history
                if any(i.index>=len(current) or current[i.index]!=i for i in receipt.items):raise RuntimeError('Unreceived native membership')
                return epoch,receipt
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise

    async def deliver(self,observed,item):
        async with self._async_lock:
            if self.delivery_fault or not self.history.ready:raise RuntimeError('Current authenticated history required')
            epoch,prior=observed
            history=self.history.native_history
            pending=tuple(i for i in history if i not in prior.items)
            if not pending or pending[0]!=item:raise ValueError('Only the next absent receipt may be delivered')
            kind=2 if 0x020300<=item.item_id<=0x020333 and item.item_id!=0x02032d else 1
            if kind==1 and item.item_id not in (0x020209,0x02020d,0x02020e,0x02020a,0x02020f,0x020210,0x020211):
                raise ValueError('Unsupported mixed item')
            if prior.key!=self.prepared.storage.key or prior.owner_cookie!=self.prepared.cookie:raise ValueError('Foreign prior membership')
            proposal=encode(ReceiptSet(prior.key,prior.owner_cookie,prior.load_id,prior.revision+1,
                tuple(sorted(prior.items+(item,),key=lambda i:i.index))))
            self._attempt+=1
            entry=dict(attempt=self._attempt,epoch=epoch,item=asdict(item),proposal=proposal.hex(),state='reserved')
            try:
                self.record(entry)  # Must finish durably before publishing a native command.
                if not self.history.ready:raise RuntimeError('History changed before publication')
                b=self.block;b.requested_authority=self._token;b.epoch=epoch;b.attempt_sequence=self._attempt
                b.kind=kind;b.proposal_size=len(proposal);c.memmove(c.addressof(b.proposal),proposal,len(proposal))
                b.outcome=MixedOutcome()
                reply=await self._offload(self._send,5);out=MixedOutcome.from_buffer_copy(b.outcome)
                entry['outcome']={n:getattr(out,n) for n,_ in out._fields_};entry['state']='acknowledged'
                self.record(entry);self.acknowledgments.append(dict(entry))
                if (reply.status,reply.active,out.status,out.epoch,out.sequence,out.kind,out.entered,out.reserved)!=(1,1,1,epoch,self._attempt,kind,1,0):
                    raise RuntimeError('Mixed award completion uncertain; do not retry')
                return decode(proposal)
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise

    def release(self):
        with self._release_lock:
            super().release()
            if self.evidence is not None:
                self.evidence['authorizes_delivery']=True
                self.evidence['mixed_acknowledgments']=self.acknowledgments
