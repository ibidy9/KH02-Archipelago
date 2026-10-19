"""Windows transport for the combined candidate; no live CLI is provided.

The distributed candidate DLL refuses attachment before any game handle opens.
Policy input must come from the future bounded, process-local name/phase adapter.
"""
from __future__ import annotations

import ctypes as c
import hashlib
from pathlib import Path
import time

from .controller_cleanup_candidate import CleanupReceipt, VerifiedControllerCleanup

EXE_SHA256 = "A5945AF5041047F54DB67D7667390F4DC90EEE59D1C06459434DDAB57D0F339C"
MAGIC, VERSION, MESSAGE = 0x4B435254, 1, 0x8260


class RouteRule(c.Structure):
    _fields_ = [(n, c.c_uint32) for n in ("kind", "caller_rva", "enabled", "once")] + [
        (n, c.c_uint64) for n in ("source_map", "source_spawn", "target_map", "target_spawn")]


class RoutePolicy(c.Structure):
    _fields_ = [("owner_cookie", c.c_uint64), ("generation", c.c_uint64),
                ("count", c.c_uint32), ("reserved", c.c_uint32), ("rules", RouteRule * 8)]


class NativeReceipt(c.Structure):
    _fields_ = [(n, c.c_uint32) for n in ("magic", "version", "size", "sequence")] + [
        (n, c.c_uint64) for n in ("owner_cookie", "generation")] + [
        (n, c.c_uint32) for n in ("status", "active", "entry_restored", "in_flight")]


class RouteBlock(c.Structure):
    _fields_ = [(n, c.c_uint32) for n in ("magic", "version", "size", "request_sequence",
        "response_sequence", "command", "status", "active")] + [
        ("policy", RoutePolicy), ("cleanup", NativeReceipt), ("matched", c.c_uint32), ("refused", c.c_uint32)]


def sha256(path: Path) -> str:
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest().upper()


class WindowsRouteTransport:
    """Single original process handle, mapping, message hook and request stream.

    Use start/stop as CandidateSession component callbacks via asyncio.to_thread.
    After startup failure, stop MUST be called; uncertain resources remain owned.
    """

    block_type = RouteBlock
    message = MESSAGE
    mapping_prefix = "KH02APCombinedRouteCandidate"
    enabled_export = "Kh02CombinedRouteLiveEnabled"
    validate_export = "Kh02ValidateRoutePolicy"
    hook_export = "Kh02CombinedRouteCandidateHook"

    def __init__(self, dll: Path, *, dll_sha256: str, pid: int, creation_time: int,
                 window: int, policy: RoutePolicy, timeout: float = 2.0):
        if pid <= 0 or creation_time <= 0 or window <= 0 or not 0 < timeout <= 10:
            raise ValueError("Fresh process/window identity and bounded timeout required")
        self.dll_path, self.dll_hash = dll.resolve(), dll_sha256.upper()
        self.pid, self.creation_time, self.window = pid, creation_time, window
        self.policy = RoutePolicy.from_buffer_copy(policy)
        self.timeout = timeout
        self.process = self.mapping = self.view = self.hook = self.mutex = None
        self.native = self.block = self.cleanup = None
        self.thread = self.sequence = 0
        self.pending_sequence = None
        self.last_published = 0
        self.arm_sent = False
        self.started = self.closed = False
        self.kernel = self.user = None

    def _api(self):
        self.kernel = c.WinDLL("kernel32", use_last_error=True)
        self.user = c.WinDLL("user32", use_last_error=True)
        declarations = (
            (self.kernel, "OpenProcess", c.c_void_p, [c.c_uint32, c.c_int, c.c_uint32]),
            (self.kernel, "CloseHandle", c.c_int, [c.c_void_p]),
            (self.kernel, "GetProcessTimes", c.c_int, [c.c_void_p] + [c.POINTER(c.c_uint64)] * 4),
            (self.kernel, "QueryFullProcessImageNameW", c.c_int, [c.c_void_p, c.c_uint32, c.c_wchar_p, c.POINTER(c.c_uint32)]),
            (self.kernel, "WaitForSingleObject", c.c_uint32, [c.c_void_p, c.c_uint32]),
            (self.kernel, "CreateMutexW", c.c_void_p, [c.c_void_p, c.c_int, c.c_wchar_p]),
            (self.kernel, "CreateFileMappingW", c.c_void_p, [c.c_void_p, c.c_void_p, c.c_uint32, c.c_uint32, c.c_uint32, c.c_wchar_p]),
            (self.kernel, "MapViewOfFile", c.c_void_p, [c.c_void_p, c.c_uint32, c.c_uint32, c.c_uint32, c.c_size_t]),
            (self.kernel, "UnmapViewOfFile", c.c_int, [c.c_void_p]),
            (self.kernel, "FreeLibrary", c.c_int, [c.c_void_p]),
            (self.user, "GetWindowThreadProcessId", c.c_uint32, [c.c_void_p, c.POINTER(c.c_uint32)]),
            (self.user, "SetWindowsHookExW", c.c_void_p, [c.c_int, c.c_void_p, c.c_void_p, c.c_uint32]),
            (self.user, "UnhookWindowsHookEx", c.c_int, [c.c_void_p]),
            (self.user, "PostThreadMessageW", c.c_int, [c.c_uint32, c.c_uint32, c.c_size_t, c.c_ssize_t]),
        )
        for library, name, result, args in declarations:
            function = getattr(library, name)
            function.restype, function.argtypes = result, args

    def start(self) -> None:
        if self.started or self.native is not None or self.closed:
            raise RuntimeError("Create a fresh route transport before starting")
        if sha256(self.dll_path) != self.dll_hash:
            raise RuntimeError("Candidate DLL hash changed")
        self._api()
        self.native = c.WinDLL(str(self.dll_path), use_last_error=True)
        enabled = getattr(self.native, self.enabled_export)
        enabled.restype = c.c_uint32
        if enabled() != 1:
            raise RuntimeError("Candidate DLL is offline-only; no game handle opened")
        validate = getattr(self.native, self.validate_export)
        validate.argtypes, validate.restype = [c.POINTER(RoutePolicy)], c.c_uint32
        if validate(c.byref(self.policy)) != 0:
            raise RuntimeError("Invalid or unresolved combined route policy")
        self.process = self.kernel.OpenProcess(0x1000 | 0x100000, False, self.pid)
        if not self.process:
            raise c.WinError(c.get_last_error())
        created, exited, kernel, user = (c.c_uint64() for _ in range(4))
        if not self.kernel.GetProcessTimes(self.process, c.byref(created), c.byref(exited), c.byref(kernel), c.byref(user)):
            raise c.WinError(c.get_last_error())
        if created.value != self.creation_time:
            raise RuntimeError("PID now belongs to another process")
        path, size = c.create_unicode_buffer(32768), c.c_uint32(32768)
        if not self.kernel.QueryFullProcessImageNameW(self.process, 0, path, c.byref(size)):
            raise c.WinError(c.get_last_error())
        executable = Path(path.value)
        if executable.stat().st_size != 76522024 or sha256(executable) != EXE_SHA256:
            raise RuntimeError("Unsupported game executable")
        window_pid = c.c_uint32()
        self.thread = self.user.GetWindowThreadProcessId(self.window, c.byref(window_pid))
        if not self.thread or window_pid.value != self.pid or self.game_exited():
            raise RuntimeError("Fresh game window association failed")
        # Existence is the lease: keep the unowned named mutex handle until cleanup.
        c.set_last_error(0)
        self.mutex = self.kernel.CreateMutexW(None, False, f"Local\\KH02APRouteOwner-{self.pid}")
        if not self.mutex or c.get_last_error() == 183:
            raise RuntimeError("Another candidate route controller owns this process")
        c.set_last_error(0)
        self.mapping = self.kernel.CreateFileMappingW(c.c_void_p(-1), None, 4, 0, c.sizeof(self.block_type),
            f"Local\\{self.mapping_prefix}-{self.pid}")
        if not self.mapping or c.get_last_error() == 183:
            raise RuntimeError("Candidate mapping already exists or could not be created")
        self.view = self.kernel.MapViewOfFile(self.mapping, 0xF001F, 0, 0, c.sizeof(self.block_type))
        if not self.view:
            raise c.WinError(c.get_last_error())
        self.block = self.block_type.from_address(self.view)
        self.block.magic, self.block.version, self.block.size = MAGIC, VERSION, c.sizeof(self.block_type)
        self.block.policy = self.policy
        callback = c.cast(getattr(self.native, self.hook_export), c.c_void_p)
        self.hook = self.user.SetWindowsHookExW(3, callback, self.native._handle, self.thread)
        if not self.hook:
            raise c.WinError(c.get_last_error())
        self.sequence = 1
        self.arm_sent = True  # Even a posting timeout may race a later native callback.
        self.cleanup = VerifiedControllerCleanup(self, cookie=self.policy.owner_cookie,
            generation=self.policy.generation, last_sequence=self.sequence)
        reply = self._request(1, self.sequence)
        if (reply is None or reply.magic != MAGIC or reply.version != VERSION or reply.size != 48
            or reply.sequence != 1 or reply.owner_cookie != self.policy.owner_cookie
            or reply.generation != self.policy.generation or reply.status != 1
            or reply.active != 1 or reply.entry_restored != 0):
            raise RuntimeError("Native ARM not confirmed; cleanup required")
        self.started = True

    def _request(self, command: int, sequence: int) -> CleanupReceipt | None:
        if command not in (1, 2) or not self.last_published < sequence < 2**32:
            raise ValueError("Invalid or reused native request sequence")
        if self.pending_sequence is not None:
            # A timeout is not a cancellation of the game-thread callback. Never
            # overwrite command fields while the previous request can still read them.
            if self.block.response_sequence != self.pending_sequence:
                return None
            self.pending_sequence = None
        self.block.command = command
        self.block.status = 0
        self.block.request_sequence = sequence
        self.last_published = sequence
        self.pending_sequence = sequence
        if not self.user.PostThreadMessageW(self.thread, self.message, 0, 0):
            return None
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            if self.game_exited():
                return None
            if self.block.response_sequence == sequence:
                # Copy once after acknowledgment; never expose a mutable view to callers.
                reply = NativeReceipt.from_buffer_copy(self.block.cleanup)
                self.pending_sequence = None
                return CleanupReceipt(**{name: getattr(reply, name) for name, _ in reply._fields_})
            time.sleep(0.01)
        return None

    def game_exited(self) -> bool:
        if not self.process:
            return False
        result = self.kernel.WaitForSingleObject(self.process, 0)
        if result not in (0, 258):
            raise RuntimeError("Original process exit status is uncertain")
        return result == 0

    def disarm(self, sequence: int) -> CleanupReceipt | None:
        self.sequence = sequence
        return self._request(2, sequence)

    def remove_message_hook(self) -> bool:
        if self.hook and not self.user.UnhookWindowsHookEx(self.hook):
            return False
        self.hook = None
        return True

    def release(self) -> None:
        if self.hook:
            if not self.game_exited():
                raise RuntimeError("Cannot release a live message hook")
            self.hook = None
        if self.view:
            if not self.kernel.UnmapViewOfFile(self.view):
                raise c.WinError(c.get_last_error())
            self.view = self.block = None
        for name in ("mapping", "process"):
            handle = getattr(self, name)
            if handle:
                if not self.kernel.CloseHandle(handle):
                    raise c.WinError(c.get_last_error())
                setattr(self, name, None)
        if self.native:
            if not self.kernel.FreeLibrary(self.native._handle):
                raise c.WinError(c.get_last_error())
            self.native = None
        if self.mutex:
            if not self.kernel.CloseHandle(self.mutex):
                raise c.WinError(c.get_last_error())
            self.mutex = None
        self.closed = True

    def stop(self) -> None:
        if self.closed:
            return
        if self.arm_sent:
            self.cleanup.stop()
        else:
            if not self.remove_message_hook():
                raise RuntimeError("Startup rollback could not remove message transport")
            self.release()
