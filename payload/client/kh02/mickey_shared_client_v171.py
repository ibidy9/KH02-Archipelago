"""Supervise one already admitted shared owner. No legacy writers or installer.

This is the public-client composition candidate, not a replacement launcher.
The caller must finish the proven storage/startup admission before attach.
"""
import asyncio
from dataclasses import dataclass

from .goal_entry_v93_candidate import attach
from .stat_entry_v105_candidate import prepare
from pathlib import Path
from .mickey_notice_delivery_v171 import NotificationDelivery
from .mickey_notifications_v171 import NotificationQueue
from .notification_names_v115_candidate import item_name
from .mickey_receipts_v171 import build_policy
from .chest_checks_v125_candidate import CombinedChecks
from .goal_observer_v73_candidate import PublicationUnavailable


def context_type():
    # Lazy import keeps offline contract tools independent of CommonClient.
    from .goal_context_v89_candidate import GoalContextMixin
    from .mickey_notice_delivery_v171 import NotificationContextMixin
    from .controlled_observation_context_candidate import ControlledObservationContext

    class SharedContext(NotificationContextMixin, GoalContextMixin, ControlledObservationContext):
        def on_package(self, cmd, args):
            slot_data = args.get('slot_data')
            if cmd == 'Connected' and type(slot_data) is dict and slot_data.get('randomized_world_access') is False:
                # The current native composition always enforces independent
                # world access. Do not silently run a linear-world seed with it.
                self.history_fault = 'Linear world access is not supported by this runtime'
                self.invalidate_history()
                self.observe(dict(event='fault', reason=self.history_fault, delivery_enabled=False))
                return
            super().on_package(cmd, args)

    return SharedContext


@dataclass(frozen=True)
class ServiceStatus:
    state: str
    goal: str | None = None
    delivery: str | None = None


class SharedClient:
    def __init__(self, owner, *, journal_root, record,
                 goal_factory=attach, delivery_factory=None, check_factory=None,
                 query_record=lambda value: None):
        owner.query_record = query_record
        self.owner = owner
        self.context = owner.session.context
        self.lock = asyncio.Lock()
        self.closed = False
        self.faulted = False
        self.running = False
        self.goal = None
        self.notice_queue = None
        self.checks = None
        if getattr(self.context, '_shared_client', None) is not None:
            raise ValueError('This context already has a shared client')
        config, snapshot = self.context.goal_admission.current(self.context)
        policy = build_policy(snapshot.received_items())
        self.goal = goal_factory(owner, journal_root)
        try:
            if delivery_factory is None:
                self.notice_queue = NotificationQueue(Path(journal_root) / 'notifications', snapshot.key)
                self.context.notice_queue = self.notice_queue
                self.delivery = NotificationDelivery(owner, queue=self.notice_queue,
                    item_name=lambda item: 'Mickey' if item==0x020233 else item_name(self.context, item), policy=policy, record=record)
            else:
                self.delivery = delivery_factory(owner, policy=policy, record=record)
            self.checks = check_factory(owner) if check_factory else CombinedChecks(owner)
        except BaseException:
            owner.revoke()
            self.goal.close()
            if self.notice_queue is not None:
                self.context.notice_queue = None
                self.notice_queue.close()
            raise
        self.context.native_revoke = owner.revoke
        self.context._shared_client = self

    async def tick(self):
        async with self.lock:
            if self.closed or self.faulted:
                raise RuntimeError('Shared client is closed or faulted')
            try:
                # Capture retained victory while offline too. This only reads
                # the existing observation mapping and writes its scoped proof.
                if not self.owner.game_exited():
                    observer = self.goal.observer
                    try:
                        observer.poll()
                    except PublicationUnavailable:
                        pass
            except BaseException:
                self.owner.revoke()
                self.faulted = True
                raise
            # Ordinary disconnect is not an uncertain delivery. Revoke before
            # waiting for a fresh complete history; never reuse a cached policy.
            if self.context.history_fault is not None:
                self.owner.revoke()
                self.faulted = True
                raise ValueError(self.context.history_fault)
            try:
                config, snapshot = self.context.goal_admission.current(self.context)
            except (RuntimeError, ValueError):
                self.owner.revoke()
                return ServiceStatus('waiting_for_connection')
            try:
                # Unsupported identities fail before any goal/native publication.
                policy = build_policy(snapshot.received_items())
                if not self.context.snapshot_is_current(snapshot):
                    self.owner.revoke()
                    return ServiceStatus('waiting_for_connection')
                # The goal service journals retained victory evidence before
                # probing a new owner. It also reports durable proof after exit.
                goal = await self.goal.tick()
                if self.owner.game_exited():
                    return ServiceStatus('game_closed', goal=goal)
                if goal == 'waiting_for_owner':
                    return ServiceStatus('waiting_for_owner', goal=goal)
                if not self.context.snapshot_is_current(snapshot):
                    self.owner.revoke()
                    return ServiceStatus('waiting_for_connection', goal=goal)
                await self.checks.poll()
                if not self.context.snapshot_is_current(snapshot):
                    self.owner.revoke()
                    return ServiceStatus('waiting_for_connection', goal=goal)
                self.delivery.policy = policy
                result = await self.delivery.reconcile_one()
                return ServiceStatus('active', goal=goal, delivery=result)
            except BaseException:
                # Once native work begins, don't infer whether an interrupted
                # request committed. Preserve journals and require fresh admission.
                self.owner.revoke()
                self.faulted = True
                raise

    async def run(self, stop, *, interval=.25, observe=lambda status: None):
        if self.running or self.closed or self.faulted:
            raise RuntimeError('Shared client cannot start another supervisor')
        if type(interval) not in (int, float) or not .05 <= interval <= 5:
            raise ValueError('Bounded polling interval required')
        self.running = True
        previous = None
        try:
            while not stop.is_set():
                status = await self.tick()
                if status != previous:
                    observe(status)
                    previous = status
                if status.state == 'game_closed':
                    return status
                try:
                    await asyncio.wait_for(stop.wait(), interval)
                except asyncio.TimeoutError:
                    pass
        finally:
            self.running = False
            self.close()

    def close(self):
        if not self.closed:
            self.owner.revoke()
            self.closed = True
            if self.goal is not None:
                self.goal.close()
            if self.notice_queue is not None:
                self.context.notice_queue = None
                self.notice_queue.close()
