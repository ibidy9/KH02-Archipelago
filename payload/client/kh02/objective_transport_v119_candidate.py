"""Read-only query deferral when AP history revokes a pending read's authority.

The v116 transport and native runtime remain unchanged. Never retries grants,
malformed acknowledgements, or refusals under current authority.
"""
import asyncio
from .objective_transport_v116_candidate import ObjectiveTransport as Previous
from .receipt_set_candidate import decode


class ObjectiveTransport(Previous):
    async def probe(self):
        async with self._async_lock:
            try:
                if not await self._sync():
                    return None
                token = self._token
                snapshot = self.session.context.current_snapshot()
                self.block.requested_authority = token
                # _send validates the complete acknowledgement before returning.
                reply = await self._offload(self._send, 3)
                current = self.session.context.snapshot_is_current(snapshot)
                if reply.status != 1 or reply.active != 1:
                    diagnostic = dict(event='receipt_query_reply_v119',
                        status=reply.status, active=reply.active,
                        sequence=getattr(reply, 'sequence', None), token=token,
                        authority=int(self.block.authority),
                        acknowledged_authority=int(self.block.acknowledged_authority),
                        history_ready=self.history.ready, snapshot_current=current)
                    deferred = (reply.status == 2 and reply.active == 1
                        and not current and not self.history.ready
                        and not self.history.faulted and self.block.authority == 0
                        and self.block.acknowledged_authority == token)
                    self.query_record(dict(diagnostic, deferred=deferred))
                    if deferred or reply.status == 3:
                        return None
                    raise RuntimeError('Receipt query refused')
                if not current or not self.history.ready:
                    return None
                size = self.block.query_size
                epoch = self.block.query_epoch
                if not epoch or not 56 <= size <= 102456 or self.block.query_reserved:
                    raise RuntimeError('Invalid receipt query')
                receipt = decode(bytes(self.block.receipts[:size]))
                if receipt.key != self.prepared.storage.key or receipt.owner_cookie != self.prepared.cookie:
                    raise RuntimeError('Foreign receipt query')
                history = self.history.native_history
                if any(i.index >= len(history) or history[i.index] != i for i in receipt.items):
                    raise RuntimeError('Unreceived native membership')
                return epoch, receipt
            except (Exception, asyncio.CancelledError):
                self.revoke()
                self.delivery_fault = True
                raise
