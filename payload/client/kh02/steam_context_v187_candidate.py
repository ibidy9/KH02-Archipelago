"""Pin the real server's first authenticated identity before storage preparation.

Existing contexts require an expected key known by their test room. A normal
client learns this from RoomInfo and Connected. After the first connection,
the exact identity remains immutable for the lifetime of this context.
"""
from .ap_history_candidate import HistoryAdmission
from .goal_session_v89_candidate import GoalSlotAdmission
from .xp_context_v185_candidate import context_type
from .mickey_slot_v172_candidate import MickeyAdmission
from .xp_slot_v185_candidate import XPAdmission, parse
from .received import DeliveryKey


class SteamContext(context_type()):
    def __init__(self, server_address, password, *, observe=lambda event: None):
        # CommonContext exposes a mutable class-level set. Each client owns its tags.
        self.tags = set(self.tags) - {"DeathLink"}
        self.pinned_key = None
        from .deathlink_v249 import LinkState
        self.death_link_state=LinkState()
        self.death_link_configuration=None
        self.trap_configuration = None
        self.trap_configuration_seen = False
        self.history_fence = None
        self.history_fence_task = None
        self.history_wait_started = None
        self.pending_seed = None
        super().__init__(server_address, password,
                         expected_key=DeliveryKey('__unadmitted__', 0, 1), observe=observe)

    def invalidate_history(self):
        self.history_fence = None
        task = self.history_fence_task
        self.history_fence_task = None
        if task is not None and not task.done():
            task.cancel()
        super().invalidate_history()

    async def request_history_fence(self, connection, key):
        try:
            if self.server is not connection:
                return
            # The server processes these in order. Nonempty history is sent by
            # Sync before Retrieved. Empty history may legitimately be omitted.
            await self.send_msgs([{'cmd': 'Sync'}, {'cmd': 'Get', 'keys': [key]}])
        except Exception as error:
            if self.server is connection:
                self.history_fault = 'Could not confirm AP item history: ' + str(error)
                self.history_fence = None

    def on_package(self, cmd, args):
        if cmd == 'Connected':
            try:
                from .traps_v244 import normalize_slot
                from .deathlink_v249 import normalize_slot as normalize_death
                death_normalized, enabled=normalize_death(args['slot_data'])
                if self.death_link_configuration is not None and enabled!=self.death_link_configuration:
                    raise ValueError('Seed DeathLink setting changed')
                self.death_link_configuration=enabled
                self.death_link_state.enabled=enabled
                self.death_link_state.clear()
                normalized, config = normalize_slot(death_normalized)
                if self.trap_configuration_seen and config != self.trap_configuration:
                    raise ValueError('Seed trap settings changed')
                self.trap_configuration = config; self.trap_configuration_seen = True
                args = dict(args,slot_data=normalized)
            except (ValueError, TypeError, KeyError) as error:
                self.history_fault = str(error); self.invalidate_history()
                self.observe(dict(event='fault',reason=str(error),delivery_enabled=False))
                return
        if self.pinned_key is None and self.history_fault is None:
            try:
                if cmd == 'RoomInfo':
                    seed = args.get('seed_name')
                    if type(seed) is not str or not seed or len(seed.encode('utf-8')) > 4096:
                        raise ValueError('Server supplied an invalid seed identity')
                    self.pending_seed = self.seed_name = seed
                    return
                if cmd == 'Connected':
                    if self.pending_seed is None or self.server is None:
                        raise ValueError('Connected before room identification')
                    parse(args['slot_data'])
                    key = DeliveryKey(self.pending_seed, args['team'], args['slot'])
                    from .storage_session_candidate import scope_binding
                    scope_binding(key)  # Validate exact types and ranges.
                    self.expected_key = self.pinned_key = key
                    self.history_admission = HistoryAdmission(key)
                    self.goal_admission = GoalSlotAdmission(key)
                    self.mickey_admission = MickeyAdmission(key)
                    self.xp_admission = XPAdmission(key)
                elif cmd == 'ReceivedItems':
                    raise ValueError('Items arrived before slot authentication')
            except (ValueError, TypeError, KeyError) as error:
                self.history_fault = str(error)
                self.invalidate_history()
                self.observe(dict(event='fault', reason=str(error), delivery_enabled=False))
                return
        super().on_package(cmd, args)
        if self.history_fault is not None:
            return
        if cmd == 'Connected':
            import asyncio, time, uuid
            self.history_wait_started = time.monotonic()
            key = 'kh02-history-fence-' + uuid.uuid4().hex
            self.history_fence = (self.server, key)
            self.history_fence_task = asyncio.create_task(self.request_history_fence(self.server, key))
        elif cmd == 'Retrieved' and self.history_fence is not None:
            connection, key = self.history_fence
            if connection is not self.server or key not in args.get('keys', {}):
                return
            self.history_fence = None
            if self.server.socket is None or self.server.socket.closed:
                return
            try:
                self.history_admission.snapshot()
            except RuntimeError:
                try:
                    # Never erase earlier receipts: receive() rejects a shrinking
                    # history, including an empty response on a nonempty reconnect.
                    self.history_admission.receive(connection, 0, ())
                    self.sync_required = False
                    self.observe(dict(event='empty_history_confirmed', delivery_enabled=False))
                except ValueError as error:
                    self.history_fault = str(error)
                    self.invalidate_history()

    def on_deathlink(self,data):
        try:snapshot=self.current_snapshot()
        except (RuntimeError,ValueError):return
        if self.death_link_state.receive(data,snapshot.connection):
            super().on_deathlink(data)
