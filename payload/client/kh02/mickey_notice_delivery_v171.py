"""Wrap proven StatDelivery. Notification success never supplies item membership."""
import asyncio
from .mickey_delivery_v171 import StatDelivery
from .mickey_receipts_v171 import build_policy
from .notification_transport_v115_candidate import read_custom
from .notification_names_v115_candidate import item_name as notification_item_name

class NotificationDelivery:
    def __init__(self,transport,*,queue,item_name,inner=None,policy=None,record=None,diagnostic=read_custom):
        self.transport=transport;self.queue=queue;self.item_name=item_name;self.diagnostic=diagnostic
        self.inner=inner or StatDelivery.from_transport(transport,policy=policy,record=record)
        self.pending=None;self.lock=asyncio.Lock();self.faulted=False
    @property
    def policy(self):return self.inner.policy
    @policy.setter
    def policy(self,value):self.inner.policy=value

    async def reconcile_one(self):
        async with self.lock:
            if self.faulted:raise RuntimeError('Notification supervisor faulted')
            ctx=self.transport.session.context
            snapshot=ctx.current_snapshot()
            observed=await self.transport.probe()
            if observed is None or not ctx.snapshot_is_current(snapshot):return None
            epoch,membership=observed
            self.queue.observe(snapshot,membership,self.item_name)
            self.inner.policy=build_policy(snapshot.received_items())
            if self.pending is not None:
                value=self.diagnostic(int(self.transport.proof.pid))
                if value['identity']!=self.pending:raise ValueError('Foreign notification acknowledgment')
                if value['pending']:return 'notification_wait'
                if value['status'] not in (7,20,21):raise ValueError('Uncertain notification completion')
                self.queue.finish(self.pending,{7:'displayed',20:'cancelled',21:'refused'}[value['status']])
                self.pending=None
                return 'notification_finished'
            notice=self.queue.next()
            if notice:
                if not ctx.snapshot_is_current(snapshot):return None
                # Durable claim BEFORE publishing; an uncertain/crashed attempt
                # is retained, never automatically repeated or made a receipt.
                self.queue.claim(notice['id']);self.pending=notice['id']
                try:await self.transport.show_notice(identity=notice['id'],epoch=epoch,text=notice['text'],ap_icon=notice['event'].startswith('o:'))
                except BaseException:
                    self.transport.revoke();self.faulted=True;raise
                return 'notification_submitted'
            return await self.inner.reconcile_one()

class NotificationContextMixin:
    """Structured server ItemSend only; ordinary chat cannot create popups.

    Bind notice_queue after shared-owner admission. Unsupported/early packets
    stay in the client's regular log and never authorize game operations.
    """
    notice_queue=None
    def on_package(self,cmd,args):
        super().on_package(cmd,args)
        if cmd!='PrintJSON' or args.get('type')!='ItemSend' or self.notice_queue is None:return
        try:
            snapshot=self.current_snapshot()
            self.notice_queue.outgoing(snapshot,args,known_locations=self.server_locations,
                item_name=lambda item,slot:notification_item_name(self,item,slot),
                player_name=lambda slot:self.player_names.get(slot,f'Player {slot}'))
        except (ValueError,RuntimeError,TypeError) as error:
            self.observe(dict(event='notification_rejected',reason=str(error)))
