"""Durable per-AP-scope difficulty preference and truthful client status.

Only the explicit player setting is persisted. Native authority, process
identity and load epochs are launch-local and are never written to disk.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from enum import IntEnum
import json
import os
from pathlib import Path

from .received import DeliveryKey

PREFERENCE_VERSION = 1


class Difficulty(IntEnum):
    BEGINNER = 0
    STANDARD = 1
    PROUD = 2
    CRITICAL = 3

    @classmethod
    def parse(cls, value: str) -> "Difficulty":
        if type(value) is not str:
            raise ValueError("Difficulty must be Beginner, Standard, Proud, or Critical")
        try:
            return cls[value.strip().upper()]
        except KeyError as error:
            raise ValueError("Difficulty must be Beginner, Standard, Proud, or Critical") from error


@dataclass(frozen=True)
class DifficultyPreference:
    key: DeliveryKey
    revision: int
    desired: Difficulty


@dataclass(frozen=True)
class PendingDifficulty:
    preference: DifficultyPreference
    process_cookie: int
    requested_epoch: int
    applied_epoch: int | None = None

    @property
    def applied(self) -> bool:
        return self.applied_epoch is not None


@dataclass(frozen=True)
class DifficultyClientStatus:
    requested: Difficulty | None
    revision: int | None
    observed_primary: Difficulty | None
    observed_checkpoint: Difficulty | None
    native_phase: int
    queued: bool
    applied: bool
    current_matches: bool
    applied_epoch: int | None

    @property
    def message(self) -> str:
        requested = "not set" if self.requested is None else self.requested.name.title()
        if self.observed_primary is None:
            observed = "unavailable"
        elif self.observed_primary == self.observed_checkpoint:
            observed = self.observed_primary.name.title()
        else:
            observed = (self.observed_primary.name.title() + " primary / " +
                        self.observed_checkpoint.name.title() + " checkpoint")
        state = ("confirmed on the current save" if self.current_matches else
                 "queued for the next authenticated known load" if self.queued else
                 "applied; awaiting current checkpoint observation" if self.native_phase==2 and self.applied else
                 "previously applied, but not observed on the current save" if self.applied else
                 "not currently confirmed")
        return f"Requested: {requested}; observed: {observed}; {state}."


class DifficultyPreferenceStore:
    def __init__(self, directory: Path):
        self.directory = Path(directory)

    def _path(self, key: DeliveryKey) -> Path:
        if not isinstance(key, DeliveryKey):
            raise ValueError("Explicit AP scope required")
        return self.directory / key.filename

    def load(self, key: DeliveryKey) -> DifficultyPreference | None:
        path = self._path(key)
        if not path.exists():
            return None
        value = json.loads(path.read_text(encoding="utf-8"))
        expected = {"version", "seed_name", "team", "slot", "revision", "desired"}
        if type(value) is not dict or set(value) != expected or value["version"] != PREFERENCE_VERSION:
            raise ValueError("Unsupported difficulty preference")
        if (value["seed_name"], value["team"], value["slot"]) != (key.seed_name, key.team, key.slot):
            raise ValueError("Difficulty preference belongs to another AP scope")
        revision, desired = value["revision"], value["desired"]
        if type(revision) is not int or not 0 < revision < 2**64 or type(desired) is not int:
            raise ValueError("Invalid difficulty preference")
        try:
            choice = Difficulty(desired)
        except ValueError as error:
            raise ValueError("Invalid difficulty preference") from error
        return DifficultyPreference(key, revision, choice)

    def choose(self, key: DeliveryKey, desired: Difficulty) -> DifficultyPreference:
        if not isinstance(desired, Difficulty):
            raise ValueError("Explicit difficulty required")
        previous = self.load(key)
        revision = 1 if previous is None else previous.revision + 1
        if revision >= 2**64:
            raise ValueError("Difficulty preference revision exhausted")
        preference = DifficultyPreference(key, revision, desired)
        self.directory.mkdir(parents=True, exist_ok=True)
        path = self._path(key);temporary = path.with_suffix(path.suffix + ".tmp")
        payload = {"version": PREFERENCE_VERSION, "seed_name": key.seed_name,
                   "team": key.team, "slot": key.slot, "revision": revision,
                   "desired": int(desired)}
        with temporary.open("x", encoding="utf-8") as stream:
            stream.write(json.dumps(payload, sort_keys=True, indent=2) + "\n")
            stream.flush();os.fsync(stream.fileno())
        try:
            os.replace(temporary, path)
        finally:
            if temporary.exists():
                temporary.unlink()
        return preference


def request(key: DeliveryKey, desired: Difficulty, *, process_cookie: int,
            epoch: int, previous: DifficultyPreference | None = None) -> PendingDifficulty:
    if not isinstance(key, DeliveryKey) or not isinstance(desired, Difficulty):
        raise ValueError('Explicit scope and difficulty required')
    for value in (process_cookie, epoch):
        if type(value) is not int or not 0 < value < 2**64:
            raise ValueError('Authenticated process and load identity required')
    if previous is not None and previous.key != key:
        raise ValueError('Difficulty preference belongs to another AP scope')
    revision = 1 if previous is None else previous.revision + 1
    if not 0 < revision < 2**64:
        raise ValueError('Difficulty preference revision exhausted')
    return PendingDifficulty(DifficultyPreference(key, revision, desired), process_cookie, epoch)


def confirm(pending: PendingDifficulty, *, key: DeliveryKey, process_cookie: int,
            revision: int, epoch: int, observed: Difficulty) -> PendingDifficulty:
    if (pending.applied or key != pending.preference.key or
            process_cookie != pending.process_cookie or
            revision != pending.preference.revision or
            type(epoch) is not int or not pending.requested_epoch < epoch < 2**64 or
            not isinstance(observed, Difficulty) or observed != pending.preference.desired):
        raise ValueError('Difficulty confirmation is stale, foreign, or not applied')
    return replace(pending, applied_epoch=epoch)


def decode_observed(value: int) -> tuple[Difficulty, Difficulty] | None:
    if type(value) is not int or value & 0xFF != 1 or value >> 24:
        return None
    try:
        return Difficulty((value >> 8) & 0xFF), Difficulty((value >> 16) & 0xFF)
    except ValueError:
        return None


def client_status(preference: DifficultyPreference | None, native: dict,
                  *, current_epoch: int | None) -> DifficultyClientStatus:
    if preference is not None and not isinstance(preference, DifficultyPreference):
        raise ValueError("Typed difficulty preference required")
    if type(native) is not dict or type(native.get("phase")) is not int:
        raise ValueError("Typed native difficulty status required")
    observed = decode_observed(native.get("observed"))
    primary, checkpoint = observed if observed is not None else (None, None)
    desired = preference.desired if preference is not None else None
    matches = desired is not None and primary == desired and checkpoint == desired
    phase = native["phase"]
    applied_epoch = native.get("applied_epoch") if type(native.get("applied_epoch")) is int and native.get("applied_epoch") else None
    same_request=preference is not None and native.get("desired")==int(desired)
    queued=bool(same_request and phase==1)
    applied=bool(same_request and phase in (2,3) and applied_epoch is not None)
    # Historical phase3/applied_epoch is not proof for the currently loaded save.
    if current_epoch is not None and (type(current_epoch) is not int or current_epoch <= 0):
        raise ValueError("Current known load epoch required")
    return DifficultyClientStatus(desired, preference.revision if preference else None,
        primary, checkpoint, phase, queued, applied, matches, applied_epoch)


class DifficultyPreferenceController:
    """Client-facing selection/status over the already-proven native command."""
    def __init__(self, store: DifficultyPreferenceStore, key: DeliveryKey):
        self.store, self.key = store, key

    def choose(self, value: Difficulty | str) -> DifficultyPreference:
        desired = Difficulty.parse(value) if type(value) is str else value
        return self.store.choose(self.key, desired)

    async def status(self, transport, *, epoch: int) -> DifficultyClientStatus:
        return client_status(self.store.load(self.key), await transport.query_difficulty(), current_epoch=epoch)

    async def reconcile(self, transport, *, epoch: int) -> DifficultyClientStatus:
        preference = self.store.load(self.key);native = await transport.query_difficulty()
        view = client_status(preference, native, current_epoch=epoch)
        if preference is None or view.current_matches:
            return view
        same_pending=(native.get("phase") in (1,2) and
                      native.get("desired")==int(preference.desired))
        if not same_pending:
            native = await transport.queue_difficulty(int(preference.desired), epoch)
            view = client_status(preference, native, current_epoch=epoch)
        return view

    async def command(self, argument: str, transport, *, epoch: int) -> DifficultyClientStatus:
        """Handle the client-facing `difficulty` argument.

        `status` is read-only. A named choice is durably stored first and then
        queued for a later authenticated known load when the current save does
        not already match. It never claims an immediate combat-time change.
        """
        if type(argument) is not str:
            raise ValueError("Use difficulty status|beginner|standard|proud|critical")
        value=argument.strip()
        if value.lower() in ("", "status"):
            return await self.status(transport,epoch=epoch)
        self.choose(value)
        return await self.reconcile(transport,epoch=epoch)
