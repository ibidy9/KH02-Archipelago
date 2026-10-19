"""Compose typed policy receipts with the proven controlled delivery lane."""
from __future__ import annotations

import asyncio
from dataclasses import asdict

from .production_policy import PolicyBinding, proposed_receipt_set
from .mickey_receipts_v171 import plan_receipts
from .wayfinder_command_v92_candidate import make_policy_command
from .receipt_set_candidate import ReceiptSet


class StatDelivery:
    """One serialized reconciliation step over commands 3, 5 and new command 6.

    ``publish_policy`` must synchronously return the native command-6 status.
    The existing transport remains solely responsible for native consumable and
    wardrobe and Zodiac awards.  Unknown acknowledgements fault both lanes.
    """

    def __init__(self, transport, *, policy, process_cookie: int,
                 scope_binding: bytes, publish_policy, record):
        if not callable(publish_policy) or not callable(record):
            raise ValueError("Policy publisher and durable recorder required")
        self.transport=transport;self.policy=policy;self.process_cookie=process_cookie
        self.scope_binding=scope_binding;self.publish_policy=publish_policy
        self.record=record;self._lock=asyncio.Lock();self.revision=0
        self.sequence=0;self.faulted=False

    @classmethod
    def from_transport(cls, transport, *, policy, record):
        """Bind only to values authenticated by the admitted native owner."""
        process_cookie,scope_binding=transport.policy_binding_values()
        return cls(transport,policy=policy,process_cookie=process_cookie,
                   scope_binding=scope_binding,publish_policy=transport.publish_policy,
                   record=record)

    async def reconcile_one(self):
        async with self._lock:
            if self.faulted:raise RuntimeError("Production delivery is faulted")
            try:
                observed=await self.transport.probe()
                if observed is None:return None
                epoch,prior=observed
                plan=plan_receipts(self.policy,prior)
                if plan.policy_receipts:
                    self.revision+=1;self.sequence+=1
                    authority=self.transport._token
                    binding=PolicyBinding(prior.owner_cookie,self.process_cookie,authority,
                                          self.revision,self.scope_binding)
                    proposed=proposed_receipt_set(prior,plan)
                    command=make_policy_command(sequence=self.sequence,generation=epoch,
                        binding=binding,policy=self.policy,membership=proposed)
                    entry=dict(kind="policy",sequence=self.sequence,revision=self.revision,
                               epoch=epoch,items=[asdict(item) for item in plan.policy_receipts],
                               state="reserved")
                    self.record(entry)
                    status=await asyncio.to_thread(self.publish_policy,command)
                    if status!=1:raise RuntimeError("Policy command completion uncertain")
                    entry["state"]="acknowledged";self.record(entry)
                    return "policy"
                if plan.native_item is not None:
                    delivered = await self.transport.deliver(observed,plan.native_item)
                    return "native" if delivered is not None else None
                return "complete"
            except (Exception,asyncio.CancelledError):
                self.transport.revoke();self.faulted=True;raise
