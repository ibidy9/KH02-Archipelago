"""Three fixed development actions through the shared game-thread owner."""
import asyncio
import ctypes as c
import mmap


class Block(c.Structure):
    _fields_ = [(n,c.c_uint32) for n in ('magic','version','size','request','response','command','status','result')] + [
        (n,c.c_uint64) for n in ('cookie','epoch','owner','manager','name')] + [
        (n,c.c_uint32) for n in ('before','after','mode','attempts')] + [('text',c.c_char*128)]


assert c.sizeof(Block)==216


class MickeyControl:
    def __init__(self,owner):
        owner._check_session()
        self.owner=owner
        self.memory=mmap.mmap(-1,216,tagname=f'Local\\KH02APMickeyControlV168-{owner.proof.pid}')
        self.block=Block.from_buffer(self.memory)
        if (self.block.magic,self.block.version,self.block.size)!=(0x38364b4d,1,216):
            self.close()
            raise RuntimeError('Native Mickey control is not installed')

    async def request(self,command):
        if command not in (1,2):raise ValueError('Unsupported fixed action')
        await self.owner.read_progress()
        status=self.owner.meter_access_status()
        if not status['owner'] or status['uncertain']:raise RuntimeError('No admitted owner')
        b=self.block
        try:
            if b.request!=b.response:raise RuntimeError('Pending action must not be overwritten')
            b.cookie=self.owner.prepared.cookie;b.epoch=status['epoch'];b.command=command
            seq=b.request+1;b.request=seq
            # A successful admitted probe explicitly services this fixed request.
            await self.owner.probe()
            for _ in range(100):
                self.owner._check_session()
                if b.response==seq:
                    return {n:(bytes(b.text).decode('ascii') if n=='text' else int(getattr(b,n))) for n,_ in b._fields_}
                await asyncio.sleep(.05)
            raise TimeoutError('Mickey action uncertain; do not resend')
        finally:del b

    def close(self):
        del self.block
        self.memory.close()
