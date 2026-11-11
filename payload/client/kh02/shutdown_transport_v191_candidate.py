"""Recognize a missing query reply across destruction of the admitted window."""

import ctypes as c
import struct
from .level_transport_v186_candidate import LevelTransport


def progress_query_action(owner, command):
    """Recognize only the existing authenticated snapshot requests, never mutations."""
    if command != 11:
        return None
    block = getattr(owner, "block", None)
    if block is None or block.proposal_size != 40:
        return None
    packet = bytes(block.proposal[:40])
    if len(packet) != 40:
        return None
    magic, epoch, cookie, load, action, reserved = struct.unpack("<4Q2I", packet)
    if (
        magic == 0x3631314F5041
        and epoch > 0
        and load > 0
        and cookie == owner.prepared.cookie
        and action in (2, 4, 11)
        and reserved == 0
    ):
        return action
    return None


class GameConnectionEnded(RuntimeError):
    """Original window disappeared; cleanup must still await process exit."""


class ShutdownTransport(LevelTransport):
    def _window_identity(self):
        user = c.WinDLL("user32", use_last_error=True)
        user.IsWindow.argtypes = [c.c_void_p]
        user.IsWindow.restype = c.c_int
        user.GetWindowThreadProcessId.argtypes = [c.c_void_p, c.POINTER(c.c_ulong)]
        user.GetWindowThreadProcessId.restype = c.c_ulong
        if not user.IsWindow(self.window):
            return None
        pid = c.c_ulong()
        thread = int(user.GetWindowThreadProcessId(self.window, c.byref(pid)))
        return pid.value, thread

    def _request(self, command, sequence):
        # Classification cannot authorize work or weaken an acknowledged reply.
        query = command == 3 or progress_query_action(self, command) is not None
        before = self._window_identity() if query else None
        if query and before is None:
            # The admitted window may already be gone before this tick starts.
            # Do not submit anything to a dead/reused window. Cleanup still
            # waits for the original process; this is not permission to retry.
            self.revoke()
            self.shutdown_interruption = dict(
                command=command,
                sequence=sequence,
                pending=self.pending_sequence,
                original_pid=self.pid,
                original_thread=self.thread,
                original_window=self.window,
                phase="before_query",
            )
            raise GameConnectionEnded("Original game window closed before a status query")
        reply = super()._request(command, sequence)
        if (
            query
            and reply is None
            and before == (self.pid, self.thread)
            and self._window_identity() is None
        ):
            self.revoke()
            self.shutdown_interruption = dict(
                command=command,
                sequence=sequence,
                pending=self.pending_sequence,
                original_pid=self.pid,
                original_thread=self.thread,
                original_window=self.window,
            )
            raise GameConnectionEnded("Original game window closed during a status query")
        return reply
