"""Sibling transport: Wayfinder exclusion retained; kind3 admits the exact twelve Zodiacs.
The original transaction body is retained verbatim; source hashes recorded.
"""
from .controlled_transport_candidate import *
from .goal_config_v80_candidate import WindowsGoalConfigTransport
from .mickey_receipts_v171 import HISTORY_ONLY

class AreaMapTransport(WindowsGoalConfigTransport):
    async def deliver(self,observed,item):
        async with self._async_lock:
            if self.delivery_fault or not self.history.ready:raise RuntimeError('Current authenticated history required')
            epoch,prior=observed
            history=self.history.native_history
            pending=tuple(i for i in history if i not in prior.items and i.item_id not in HISTORY_ONLY)
            if not pending or pending[0]!=item:raise ValueError('Only the next absent receipt may be delivered')
            kind=2 if 0x020300<=item.item_id<=0x020333 and item.item_id!=0x02032d else 1
            if 0x020223<=item.item_id<=0x02022e:kind=3
            if 0x02022f<=item.item_id<=0x020232:kind=4
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
                # Exact no-effect acknowledgment from the candidate runtime.
                # Sequence is consumed; membership is not. Re-probe next tick.
                if (reply.status,reply.active,out.status,out.epoch,out.sequence,out.kind,
                    out.entered,out.before,out.after,out.reserved)==(0x273,1,0x273,epoch,
                    self._attempt,kind,0,0xffffffff,0xffffffff,0):
                    return None
                if (reply.status,reply.active,out.status,out.epoch,out.sequence,out.kind,out.entered,out.reserved)!=(1,1,1,epoch,self._attempt,kind,1,0):
                    raise RuntimeError('Mixed award completion uncertain; do not retry')
                return decode(proposal)
            except (Exception,asyncio.CancelledError):
                self.revoke();self.delivery_fault=True;raise

