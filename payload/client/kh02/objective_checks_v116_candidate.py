"""Objective check reporting only; no game reads, rewards, or save mutations.

The native adapter must obtain these states from the admitted shared owner.
This candidate is deliberately not attached to the legacy process watcher.
"""
import asyncio
from dataclasses import dataclass

from .objectives import objective_location_id


@dataclass(frozen=True)
class ObjectiveObservation:
    cookie: int
    epoch: int
    load_id: int
    owner: int
    suppressed: bool
    states: tuple[int, ...]

    def completed(self):
        if (any(type(v) is not int or not 0 < v < 2**64
                for v in (self.cookie, self.epoch, self.load_id, self.owner))
                or self.suppressed is not True
                or type(self.states) is not tuple or len(self.states) != 51
                or any(type(v) is not int or not 0 <= v <= 0xffffffff
                       or (v & 15) > 2 for v in self.states)):
            raise ValueError('Invalid admitted objective observation')
        # Objective 51 is goal-only and explicitly excluded from the AP pool.
        # 42 and other option-dependent checks are filtered by server_locations.
        return frozenset(objective_location_id(i + 1)
                         for i, v in enumerate(self.states[:50]) if (v & 15) == 2)


class ObjectiveReporter:
    """Use server acknowledgement as truth; never manufacture local rewards.

    read() returns (observation, receipt_set, epoch) under the owner's existing
    transport lock. current(observation) revalidates the native binding without
    yielding. The caller supplies the authenticated scope and owner cookie.
    A check send is idempotent in AP. Unacknowledged checks are retried on later
    polls; a successful socket write alone is not recorded as completion.
    """
    def __init__(self, context, *, key, cookie, read, current):
        if type(cookie) is not int or not 0 < cookie < 2**64:
            raise ValueError('Prepared owner cookie required')
        self.context = context
        self.key = key
        self.cookie = cookie
        self.read = read
        self.current = current
        self.lock = asyncio.Lock()

    async def poll(self):
        async with self.lock:
            ctx = self.context
            snapshot = ctx.current_snapshot()
            if snapshot.key != self.key:
                raise ValueError('Foreign AP check scope')
            value = await self.read()
            if value is None or not ctx.snapshot_is_current(snapshot):
                return ()
            observation, membership, epoch = value
            locations = observation.completed()
            if (membership.key != self.key or membership.owner_cookie != self.cookie
                    or observation.cookie != self.cookie or observation.epoch != epoch
                    or observation.load_id != membership.load_id):
                raise ValueError('Objective and receipt owner binding differ')
            pending = tuple(sorted((locations & set(ctx.server_locations))
                                   - set(ctx.checked_locations)))
            if (not pending or not self.current(observation)
                    or not ctx.snapshot_is_current(snapshot)):
                return ()
            # Actual received rewards and outgoing notifications remain entirely
            # owned by ReceivedItems / ItemSend and the proven v115 dispatcher.
            await ctx.check_locations(pending)
            return pending
