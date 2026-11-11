"""Wait for UE4SS host initialization before requesting authenticated startup proof."""

import json
from pathlib import Path
import re
import time

WINDOW = 256 * 1024
FILETIME_EPOCH = 116444736000000000


def log_windows(stream, size):
    """Keep the startup banner and the latest complete lines, even on long retries."""
    head = stream.read(WINDOW)
    if size <= WINDOW:
        return head.decode("utf-8", errors="replace")
    if size <= 2 * WINDOW:
        return (head + stream.read(WINDOW)).decode("utf-8", errors="replace")
    stream.seek(size - WINDOW)
    # Drop partial lines at the cut so marker text cannot join across the gap.
    head = head.rpartition(b"\n")[0]
    tail = stream.read(WINDOW).partition(b"\n")[2]
    return (head + b"\n" + tail).decode("utf-8", errors="replace")


class ReceiptStartupReader:
    def __init__(self, reader, log, *, diagnostic_path=None, notify=None):
        self.reader = reader
        self.log = Path(log)
        self.diagnostic_path = diagnostic_path
        self.notify = notify
        self.last_state = None
        self.wait_started = None
        self.reported_delay = False
        self.locations = {}

    def record(self, state, game, age_ns=None, **details):
        # Keep one current report and write only when the state changes.
        binary_dir = self.reader.executable.parent
        loaded = {Path(row["path"]) for row in game.get("modules", ())}
        locations = {}
        for relative in (
            "UE4SS.dll",
            "dwmapi.dll",
            "dinput8.dll",
            "Mods/KH02ReceiptHost/dlls/main.dll",
        ):
            expected = binary_dir / relative
            other = any(path.name.casefold() == expected.name.casefold() for path in loaded)
            locations[relative] = (
                "expected" if expected in loaded else "elsewhere" if other else "unobserved"
            )
        self.locations = locations
        identity = (game["pid"], game["created"], state, tuple(locations.values()))
        if self.last_state == identity:
            return
        self.last_state = identity
        if self.diagnostic_path:
            report = dict(
                state=state,
                pid=game["pid"],
                executable=self.reader.executable.name,
                loaded_modules=locations,
                log_age_relative_to_launch_ms=None if age_ns is None else age_ns // 1000000,
                **details,
            )
            try:
                target = Path(self.diagnostic_path)
                temporary = target.with_suffix(".tmp")
                temporary.write_text(json.dumps(report, indent=2), encoding="utf-8")
                temporary.replace(target)
            except OSError:
                pass  # A support report is never part of admission.

    def observe(self, preparation, *, games=None):
        if games is None:
            raise ValueError("Use the coordinator exact process inventory")
        if len(games) != 1 or Path(games[0]["path"]).resolve() != self.reader.executable:
            return self.reader.observe(preparation, games=games)

        game = games[0]
        if self.wait_started is None:
            self.wait_started = time.monotonic()
        age = None
        state = "missing_log"
        try:
            stat = self.log.stat()
            age = stat.st_mtime_ns - (game["created"] - FILETIME_EPOCH) * 100
            state = "stale_log"
            if age >= 0:
                with self.log.open("rb") as stream:
                    text = log_windows(stream, stat.st_size)
                marker = f'[KH02ReceiptBridge] READY pid={game["pid"]} abi=1'
                if marker in text and "[KH02ReceiptLua] REGISTERED" in text:
                    self.record("host_ready", game, age)
                    return self.reader.observe(preparation, games=games)
                if "Fatal Error: PS scan timed out" in text:
                    attempts = re.findall(r"PS Scan attempt (\d+)", text)
                    failed = sorted(
                        set(
                            re.findall(r"\[PS\] Failed to find ([^:\r\n]+(?:::[^:\r\n]+)*): ", text)
                        )
                    )
                    self.record(
                        "ps_scan_timeout",
                        game,
                        age,
                        last_attempt=attempts[-1] if attempts else None,
                        reported_failed_resolvers=failed,
                        log_bytes=stat.st_size,
                    )
                    reason = (
                        ", ".join(failed)
                        if failed
                        else "no individual lookup was identified in the available log"
                    )
                    raise RuntimeError(
                        "UE4SS timed out looking up engine addresses: "
                        + reason
                        + ". Close the game normally. The AP session directory contains the startup report."
                    )
                if (
                    "No mods.txt file found" in text
                    and "Error: directory_iterator::directory_iterator:" in text
                ):
                    self.record("missing_mod_folder", game, age)
                    raise RuntimeError(
                        "UE4SS could not open its Mods folder. Check Binaries/Win64/Mods and UE4SS-settings.ini in the 0.2 installation. Close the game before correcting the mod files."
                    )
                state = "waiting_for_host"
        except FileNotFoundError:
            state = "missing_log"
        except PermissionError:
            state = "unreadable_log"
        self.record(state, game, age)
        if self.notify and not self.reported_delay and time.monotonic() - self.wait_started >= 45:
            self.reported_delay = True
            self.notify(
                "0.2 was detected. Still waiting for UE4SS startup ("
                + state.replace("_", " ")
                + "). UE4SS.dll: "
                + self.locations["UE4SS.dll"]
                + ". AP host: "
                + self.locations["Mods/KH02ReceiptHost/dlls/main.dll"]
                + ". Stay at title."
            )
        return None
