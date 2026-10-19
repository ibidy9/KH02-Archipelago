"""CommonClient history adapter with no game reader, watcher or delivery calls."""
import os
from dataclasses import asdict
os.environ.setdefault('SKIP_REQUIREMENTS_UPDATE', '1')

from CommonClient import CommonContext
from .ap_history_candidate import HistoryAdmission, NetworkReceipt
from .received import DeliveryKey


class ObservationContext(CommonContext):
    game = 'Kingdom Hearts 0.2 Birth by Sleep'
    items_handling = 0b111

    def __init__(self, server_address, password, *, expected_key, observe=lambda event: None):
        self.history_admission = HistoryAdmission(expected_key)
        self.expected_key = expected_key
        self.observe = observe
        self.history_fault = None
        self.sync_required = False
        super().__init__(server_address, password)

    def invalidate_history(self):
        self.history_admission.disconnect()
        self.sync_required = False

    def reset_server_state(self):
        self.invalidate_history()
        super().reset_server_state()

    async def disconnect(self, allow_autoreconnect=False):
        self.invalidate_history()  # Before any await, including slow socket shutdown.
        await super().disconnect(allow_autoreconnect)

    async def connection_closed(self):
        self.invalidate_history()
        await super().connection_closed()

    def on_package(self, cmd, args):
        if self.history_fault is not None:
            return
        try:
            if cmd == 'RoomInfo':
                if args.get('seed_name') != self.expected_key.seed_name:
                    raise ValueError('Unexpected AP room')
                self.seed_name = args['seed_name']
            elif cmd == 'Connected':
                key = DeliveryKey(self.seed_name, args['team'], args['slot'])
                self.history_admission.connect(self.server, key)
                self.sync_required = True
                self.observe(dict(event='connected', delivery_enabled=False))
            elif cmd == 'ReceivedItems':
                items = tuple(NetworkReceipt(*item) for item in args['items'])
                ready = self.history_admission.receive(self.server, args['index'], items)
                self.sync_required = not ready
                self.observe(dict(event='history', index=args['index'], count=len(items),
                                  items=[asdict(item) for item in items],
                                  ready=ready, delivery_enabled=False))
        except (ValueError, TypeError, KeyError) as error:
            self.history_fault = str(error)
            self.invalidate_history()
            self.observe(dict(event='fault', reason=str(error), delivery_enabled=False))

    def current_snapshot(self):
        if (self.history_fault is not None or self.server is None
                or self.server.socket is None or self.server.socket.closed):
            raise RuntimeError('AP history admission is closed')
        snapshot = self.history_admission.snapshot()
        if snapshot.connection is not self.server:
            raise RuntimeError('AP connection changed')
        return snapshot

    def snapshot_is_current(self, snapshot):
        return (self.history_fault is None and self.server is snapshot.connection
                and self.server is not None and self.server.socket is not None
                and not self.server.socket.closed
                and self.history_admission.is_current(snapshot))

    async def server_auth(self, password_requested=False):
        if password_requested and not self.password:
            await super().server_auth(password_requested)
        await self.get_username()
        await self.send_connect()
