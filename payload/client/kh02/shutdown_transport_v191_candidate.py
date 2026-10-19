"""Recognize a missing query reply across destruction of the admitted window."""
import ctypes as c
from .level_transport_v186_candidate import LevelTransport


class GameConnectionEnded(RuntimeError):
    """Original window disappeared; cleanup must still await process exit."""


class ShutdownTransport(LevelTransport):
    def _window_identity(self):
        user=c.WinDLL('user32',use_last_error=True)
        user.IsWindow.argtypes=[c.c_void_p];user.IsWindow.restype=c.c_int
        user.GetWindowThreadProcessId.argtypes=[c.c_void_p,c.POINTER(c.c_ulong)]
        user.GetWindowThreadProcessId.restype=c.c_ulong
        if not user.IsWindow(self.window):return None
        pid=c.c_ulong()
        thread=int(user.GetWindowThreadProcessId(self.window,c.byref(pid)))
        return pid.value,thread

    def _request(self,command,sequence):
        # Classification cannot authorize work or weaken an acknowledged reply.
        before=self._window_identity() if command==3 else None
        reply=super()._request(command,sequence)
        if (command==3 and reply is None and before==(self.pid,self.thread)
                and self._window_identity() is None):
            self.revoke()
            self.shutdown_interruption=dict(command=command,sequence=sequence,
                pending=self.pending_sequence,original_pid=self.pid,
                original_thread=self.thread,original_window=self.window)
            raise GameConnectionEnded('Original game window closed during a status query')
        return reply
