"""Explicit goal lifecycle composition. No auto-install, game launch or save writes.

The caller supplies configuration captured from authenticated Connected slot
data, and invokes tick after its normal receipt service. Existing native access,
witness binding, immutable journal and scoped AP reporter remain authoritative.
"""
import asyncio
from .goal_access_candidate import Goal, TideRequirement, WORLDS
from .goal_ownership_v79_candidate import GoalConfiguration
from .goal_completion_v2_candidate import GoalCompletion
from .goal_observer_v73_candidate import GoalObserver, PublicationUnavailable


def parse_goal_slot_data(value):
    """Explicit schema2/schema5 contract; never reinterpret older seed data."""
    if type(value) is not dict:
        raise ValueError('Goal slot data must be an object')
    required={'schema_version','item_schema_version','game','goal','goal_requirement',
              'starting_world','randomized_world_access','minimal_clear_content','critical_available'}
    if set(value)!=required:
        raise ValueError('Goal slot-data fields differ from schema2')
    if (type(value['schema_version']) is not int or value['schema_version']!=2 or
        type(value['item_schema_version']) is not int or value['item_schema_version']!=5 or
        value['game']!='Kingdom Hearts 0.2 Birth by Sleep'):
        raise ValueError('Unsupported goal slot-data version')
    for key in ('randomized_world_access','minimal_clear_content','critical_available'):
        if type(value[key]) is not bool:raise ValueError('Boolean slot-data field required')
    if not value['minimal_clear_content'] or not value['critical_available']:
        raise ValueError('Required fresh AP initialization disabled')
    if type(value['starting_world']) is not str or value['starting_world'] not in WORLDS:
        raise ValueError('Unknown starting world')
    config=GoalConfiguration(5,Goal(value['goal']),TideRequirement(value['goal_requirement']),value['starting_world'])
    config.validate()
    return config


class GoalSlotAdmission:
    """Call connected only from the authenticated Connected packet handler.

    Connection identity is checked again against current admitted item history.
    A reconnect may repeat the same configuration, never change this seed's goal.
    """
    def __init__(self,key):
        self.key=key;self.connection=None;self.configuration=None

    def disconnect(self):self.connection=None

    def connected(self,connection,key,slot_data):
        self.disconnect()
        if connection is None or key!=self.key:raise ValueError('Goal room differs')
        config=parse_goal_slot_data(slot_data)
        if self.configuration is not None and config!=self.configuration:
            raise ValueError('Seed goal configuration changed')
        self.configuration=config;self.connection=connection

    def current(self,context):
        snapshot=context.current_snapshot()
        if (self.configuration is None or self.connection is None or
            snapshot.key!=self.key or snapshot.connection is not self.connection or
            not context.snapshot_is_current(snapshot)):
            raise ValueError('Goal configuration requires matching authenticated history')
        return self.configuration,snapshot


class GoalSession:
    def __init__(self,coordinator,admission,journal_root,*,observer_factory=GoalObserver):
        self.context=coordinator.context;self.owner=coordinator.transport
        self.configuration,_=admission.current(self.context)
        if (coordinator.prepared.storage.key!=admission.key or
            coordinator.prepared.slot_data.goal!=self.configuration.selected_goal.value or
            coordinator.prepared.slot_data.starting_world!=self.configuration.starting_world):
            raise ValueError('Prepared session and authenticated goal differ')
        self.admission=admission
        self.observer=observer_factory(coordinator,journal_root)
        self.reporter=GoalCompletion(self.observer.store)
        self.sent_connection=None;self.configured_epoch=None;self.closed=False
        self.lock=asyncio.Lock()

    async def tick(self):
        async with self.lock:
            if self.closed:raise ValueError('Goal session closed')
            # Journal retained evidence before probing a potentially new owner.
            # Uninitialized/torn publications are retried; disk failures propagate.
            if not self.owner.game_exited():
                try:self.observer.poll()
                except PublicationUnavailable:pass
            config,snapshot=self.admission.current(self.context)
            if config!=self.configuration:raise ValueError('Goal configuration changed')
            # Durable proof can be resent after reconnect, even after game exit.
            completed=self.observer.store.load() is not None
            if completed:
                if self.sent_connection is not snapshot.connection:
                    await self.reporter.report(self.context)
                    self.sent_connection=snapshot.connection
            # Completion is durable, but native access configuration is not.
            # A new process still needs it for world menus and goal access.
            if self.owner.game_exited():return 'completed' if completed else 'game_closed'
            if not await self.observer.bind():return 'completed' if completed else 'waiting_for_owner'
            config,snapshot=self.admission.current(self.context)
            state=self.owner.meter_access_status()
            if state['uncertain'] or not state['owner'] or not state['epoch']:
                return 'completed' if completed else 'waiting_for_owner'
            # Reconfigure after owner epoch or network connection changes. Native
            # command9 independently authenticates the current cookie and epoch.
            binding=(snapshot.connection,state['epoch'])
            if self.configured_epoch!=binding:
                await self.owner.configure_goal(config,state['epoch'])
                self.admission.current(self.context)
                self.configured_epoch=binding
            return 'completed' if completed else 'observing'

    def close(self):
        if not self.closed:
            self.closed=True;self.observer.close()
