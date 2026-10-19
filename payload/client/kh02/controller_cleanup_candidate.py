"""Explicit cleanup contract for candidate controllers; legacy wrappers unchanged."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol


MAGIC = 0x4B435254
VERSION = 1
RECEIPT_SIZE = 48


@dataclass(frozen=True)
class CleanupReceipt:
    magic: int
    version: int
    size: int
    sequence: int
    owner_cookie: int
    generation: int
    status: int
    active: int
    entry_restored: int
    in_flight: int

    def verified(self, *, cookie: int, generation: int, sequence: int) -> bool:
        return (0 < cookie < 2**64 and 0 < generation < 2**64 and 0 < sequence < 2**32
                and self.magic == MAGIC and self.version == VERSION and self.size == RECEIPT_SIZE
                and self.owner_cookie == cookie and self.generation == generation
                and self.sequence == sequence and self.status == 1
                and self.active == 0 and self.entry_restored == 1 and self.in_flight == 0)


class CleanupTransport(Protocol):
    def game_exited(self) -> bool:
        """Check the original verified PROCESS HANDLE, never a PID lookup."""

    def disarm(self, sequence: int) -> CleanupReceipt | None:
        """Bounded request/ack; return None on timeout, retaining all resources."""

    def remove_message_hook(self) -> bool:
        """Return the actual UnhookWindowsHookEx result."""

    def release(self) -> None:
        """Release local mapping/module/handles; raise if not confirmed."""


class CleanupUncertain(RuntimeError):
    pass


class VerifiedControllerCleanup:
    """No kill fallback. Ownership survives timeout, bad ack or unload failure.

    Call under CandidateSession ownership; transport requests must be serialized.
    The caller passes the last sequence actually sent, including failed requests.
    """

    def __init__(self, transport: CleanupTransport, *, cookie: int, generation: int,
                 last_sequence: int):
        if not 0 < cookie < 2**64 or not 0 < generation < 2**64 or not 0 <= last_sequence < 2**32:
            raise ValueError("Invalid controller cleanup identity")
        self.transport = transport
        self.cookie, self.generation, self.sequence = cookie, generation, last_sequence
        self.native_verified = self.transport_removed = self.closed = False
        self.outcome: str | None = None

    def stop(self) -> None:
        if self.closed:
            return
        if self.transport.game_exited():
            # An exited, originally verified process cannot retain a live detour.
            self.transport.release()
            self.closed, self.outcome = True, "verified-game-exit"
            return
        if not self.native_verified:
            if self.sequence == 2**32 - 1:
                raise CleanupUncertain("Sequence exhausted; retain controller until verified game exit")
            self.sequence += 1
            receipt = self.transport.disarm(self.sequence)
            if receipt is None or not receipt.verified(cookie=self.cookie, generation=self.generation,
                                                       sequence=self.sequence):
                raise CleanupUncertain("Native cleanup unconfirmed; controller resources retained")
            self.native_verified = True
        if not self.transport_removed:
            if not self.transport.remove_message_hook():
                raise CleanupUncertain("Message-hook removal unconfirmed; controller resources retained")
            self.transport_removed = True
        self.transport.release()
        self.closed, self.outcome = True, "verified-native-cleanup"
