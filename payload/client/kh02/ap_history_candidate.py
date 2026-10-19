"""Offline AP history admission. No network connections or game operations.

The future CommonClient adapter must supply the actual connection object on
every event, call disconnect on loss, and check snapshot validity immediately
before publication. A retained snapshot never authorizes a later connection.
"""
from dataclasses import dataclass

from .received import DeliveryKey, ReceivedItem


@dataclass(frozen=True)
class NetworkReceipt:
    item: int
    location: int
    player: int
    flags: int


@dataclass(frozen=True)
class HistorySnapshot:
    key: DeliveryKey
    connection: object
    revision: int
    items: tuple[NetworkReceipt, ...]

    def received_items(self) -> tuple[ReceivedItem, ...]:
        return tuple(ReceivedItem(i, item.item, item.player)
                     for i, item in enumerate(self.items))


class HistoryAdmission:
    def __init__(self, key: DeliveryKey):
        self.key = key
        self._connection = None
        self._revision = 0
        self._history: tuple[NetworkReceipt, ...] = ()
        self._ready = False
        self._fault = False

    def connect(self, connection: object, key: DeliveryKey) -> None:
        self.disconnect()
        if connection is None or key != self.key or self._fault:
            raise ValueError("Fresh matching AP identity required")
        self._connection = connection

    def disconnect(self) -> None:
        self._ready = False
        self._connection = None
        self._revision += 1

    def receive(self, connection: object, index: int,
                items: tuple[NetworkReceipt, ...]) -> bool:
        """Return False to request Sync; corruption permanently closes admission.

        Matches the local CommonClient index-zero replacement / tail-append
        protocol, with additional immutable-prefix checks. Never accept an old
        connection's packet as evidence about the active connection.
        """
        if connection is not self._connection or connection is None:
            raise ValueError("Stale AP connection")
        self._revision += 1
        if self._fault:
            raise ValueError("History identity is faulted")
        # Materialize once: validating a generator first would consume it and
        # could turn a nonempty packet into an admitted empty history.
        try:
            items = tuple(items)
        except (TypeError, ValueError):
            self._ready = False
            self._fault = True
            raise ValueError("Malformed ReceivedItems packet") from None
        if type(index) is not int or index < 0 or any(
            not isinstance(item, NetworkReceipt) or any(type(value) is not int for value in
                (item.item, item.location, item.player, item.flags)) for item in items
        ):
            self._ready = False
            self._fault = True
            raise ValueError("Malformed ReceivedItems packet")
        if index == 0:
            if len(items) < len(self._history) or items[:len(self._history)] != self._history:
                self._ready = False
                self._fault = True
                raise ValueError("AP history shrank or changed identity")
            self._history = items
            self._ready = True
            return True
        if not self._ready or index != len(self._history):
            self._ready = False
            return False
        self._history += items
        return True

    def snapshot(self) -> HistorySnapshot:
        if not self._ready or self._fault or self._connection is None:
            raise RuntimeError("Await authenticated index-zero history")
        return HistorySnapshot(self.key, self._connection, self._revision, self._history)

    def is_current(self, snapshot: HistorySnapshot) -> bool:
        return (self._ready and not self._fault and self._connection is not None
                and snapshot.connection is self._connection and snapshot.key == self.key
                and snapshot.revision == self._revision and snapshot.items == self._history)
