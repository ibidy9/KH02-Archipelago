"""Read only the fixed native death publication, never process memory."""
import mmap
import struct
WIRE=struct.Struct('<4I8Q')

def decode(first,second):
    if len(first)!=80 or first!=second:raise ValueError('Unstable death observation')
    v=WIRE.unpack(first)
    if v[:3]!=(0x3035444b,1,80) or v[3]&1:raise ValueError('Invalid death observation')
    return dict(zip(('serial','cookie','epoch','actor','remote','hp','state','result'),v[4:]))

class DeathObservation:
    def __init__(self,owner):
        self.owner=owner;self.memory=None;self.serial=0
    def read(self):
        if self.owner.game_exited():return None
        if self.memory is None:
            try:self.memory=mmap.mmap(-1,80,tagname=f'Local\\KH02APDeathV250-{self.owner.proof.pid}',access=mmap.ACCESS_READ)
            except OSError:return None
        try:row=decode(self.memory[:],self.memory[:])
        except ValueError:return None
        if row['cookie']!=self.owner.prepared.cookie or row['serial']<=self.serial:return None
        self.serial=row['serial']
        return row
    def close(self):
        if self.memory is not None:self.memory.close();self.memory=None
