"""Availability acknowledgement over the existing authenticated AP history.

No item effects, save adoption, connection creation, or automatic retries after
uncertain native acknowledgement. The future dispatcher must check ready before
every publication and still supply its native lifetime/authorization checks.
"""
import asyncio
from .received import DeliveryKey, ReceivedItem


def _items(values):
    values = tuple(values)
    if len(values) > 4096 or any(
        not isinstance(value, ReceivedItem) or value.index != i or any(
            type(n) is not int or not 0 <= n < 2**64
            for n in (value.index, value.item_id, value.player)
        ) for i, value in enumerate(values)
    ):
        raise ValueError('Malformed or oversized native history')
    return values


class CoordinatedHistory:
    def __init__(self, context, key: DeliveryKey, prepared_history):
        self.context = context
        self.key = key
        self.native_history = _items(prepared_history)
        self.faulted = False
        self._confirmed = None
        self._inflight = False

    @property
    def ready(self):
        return (not self.faulted and not self._inflight and self._confirmed is not None
                and self.context.snapshot_is_current(self._confirmed))

    async def synchronize(self, apply):
        """apply(full_history) awaits the bound native owner-thread coordinator.

        Native results:1 committed,3 BUSY before changes. Every other result or
        uncertain acknowledgement faults this bridge. A committed stale snapshot
        advances native availability but never authorizes effects on reconnect.
        """
        if self.faulted:
            raise RuntimeError('History coordination is faulted')
        if self._inflight:
            return False
        self._confirmed = None
        snapshot = self.context.current_snapshot()
        if not self.context.snapshot_is_current(snapshot):
            return False
        try:
            items = _items(snapshot.received_items())
            if snapshot.key != self.key or items[:len(self.native_history)] != self.native_history:
                raise ValueError('History scope or acknowledged prefix changed')
        except (TypeError, ValueError):
            self.faulted = True
            raise
        self._inflight = True
        try:
            status = await apply(items)
            if type(status) is not int or status not in (1, 3):
                raise RuntimeError('Native history update failed')
            if status == 3:
                return False
            self.native_history = items
            if not self.context.snapshot_is_current(snapshot):
                return False
            self._confirmed = snapshot
            return True
        except (Exception, asyncio.CancelledError):
            self.faulted = True
            raise
        finally:
            self._inflight = False
