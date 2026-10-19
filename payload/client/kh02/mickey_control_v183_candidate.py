"""Retain a fixed reconciliation request across an unadmitted gameplay gap.

No new native commands or retries: a later ordinary probe may acknowledge the
same pending request. A successful admitted probe must acknowledge immediately.
"""
import asyncio
from .mickey_control_v171 import MickeyControl as Previous


class MickeyControl(Previous):
    def __init__(self, owner):
        super().__init__(owner)
        self.pending = None
        self.request_lock = asyncio.Lock()

    @staticmethod
    def waiting(pending=False):
        return dict(status=3, result=40, deferred=True, pending=pending)

    async def request(self, command):
        if command not in (1, 2):
            raise ValueError('Unsupported fixed action')
        async with self.request_lock:
            b = None
            try:
                if self.pending is None:
                    progress = await self.owner.read_progress()
                    if progress is None:
                        return self.waiting()
                    status = self.owner.meter_access_status()
                    if not status['owner'] or status['uncertain']:
                        return self.waiting()
                    b = self.block
                    if b.request != b.response:
                        raise RuntimeError('Foreign pending action must not be overwritten')
                    if b.request == 0xffffffff:
                        raise RuntimeError('Mickey request sequence exhausted')
                    seq = b.request + 1
                    b.cookie = self.owner.prepared.cookie
                    b.epoch = status['epoch']
                    b.command = command
                    self.pending = (seq, command, int(b.cookie), int(b.epoch))
                    b.request = seq
                seq, original_command, cookie, epoch = self.pending
                if command != original_command:
                    raise RuntimeError('Cannot replace pending Mickey command')
                b = self.block
                if (b.request, b.command, b.cookie, b.epoch) != self.pending:
                    raise RuntimeError('Pending Mickey request identity changed')
                # This is a normal owner query, not another action publication.
                observed = await self.owner.probe()
                self.owner._check_session()
                if b.response != seq:
                    if observed is not None:
                        raise RuntimeError('Admitted Mickey probe did not acknowledge; do not resend')
                    return self.waiting(pending=True)
                result = {n: (bytes(b.text).decode('ascii') if n == 'text' else int(getattr(b, n)))
                          for n, _ in b._fields_}
                # If the refusal was acknowledged during another owner gap, keep
                # its identity until a current epoch can distinguish a transition
                # from a same-epoch refusal. Do not publish a replacement meanwhile.
                if (result['status'], result['result']) == (2, 0) and observed is None:
                    return self.waiting(pending=True)
                self.pending = None
                # Native result0 is set before admission checks and before any spawn.
                # Only an acknowledged rejection across an observed epoch change is
                # deferrable. Other native refusals remain visible to the supervisor.
                if (result['status'], result['result']) == (2, 0) and observed is not None and observed[0] != epoch:
                    return self.waiting()
                return result
            finally:
                del b
