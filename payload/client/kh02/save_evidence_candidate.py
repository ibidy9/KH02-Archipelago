"""Bounded post-exit evidence decoder; does not authorize delivery or attachment."""
import ctypes as c
import hashlib
from .receipt_set_candidate import decode,validate
class Event(c.Structure):
    _fields_=[('kind',c.c_uint32),('thread',c.c_uint32),('epoch',c.c_uint64),('result',c.c_uint64),
              ('known',c.c_uint32),('complete',c.c_uint32),('started',c.c_uint64),('finished',c.c_uint64),
              ('argument',c.c_uint64),('receipt_size',c.c_uint32),('flags',c.c_uint32),('receipts',c.c_ubyte*102456)]
class Publication(c.Structure):
    _fields_=[('started',c.c_uint64),('finished',c.c_uint64),('thread',c.c_uint32),('result',c.c_uint32),
              ('complete',c.c_uint32),('reserved',c.c_uint32),('frozen',c.c_ubyte*0x1725b0)]
assert Event.receipts.offset==64 and c.sizeof(Event)==102520
assert Publication.frozen.offset==32 and c.sizeof(Publication)==1517008

def parse_closed(events,count,overflow,publications,publication_count,*,key,cookie,history,supported):
    """Caller must prove original-process exit before supplying copied evidence.
    This checks evidence integrity, not a particular successful live sequence.
    """
    if not 0<count<=32 or overflow or count>len(events) or not 0<=publication_count<=4 or publication_count>len(publications):
        raise ValueError('Incomplete or overflowing evidence')
    result=[]
    for e in events[:count]:
        if not (1<=e.kind<=6 and e.thread and e.complete==1 and not e.flags and e.started<=e.finished
                and e.known in (0,1) and e.receipt_size<=102456):raise ValueError('Invalid event metadata')
        if e.kind in (1,4,6) and e.result not in (0,1):raise ValueError('Invalid native boolean result')
        if any(e.receipts[e.receipt_size:]):raise ValueError('Unexpected receipt tail')
        wire=bytes(e.receipts[:e.receipt_size])
        if e.known:
            receipt=decode(wire);validate(receipt,key,cookie,e.epoch,history,supported)
        elif wire:raise ValueError('Unknown event retained receipts')
        result.append(dict(kind=e.kind,thread=e.thread,epoch=e.epoch,result=e.result,known=bool(e.known),
            started=e.started,finished=e.finished,argument=e.argument,receipts_hex=wire.hex() if wire else None))
    saves=[]
    for p in publications[:publication_count]:
        if p.complete!=1 or p.reserved or not p.thread or p.started>p.finished or p.result not in (0,1):
            raise ValueError('Incomplete original publication')
        raw=bytes(p.frozen)
        if raw[:16]!=b'SAVE'+(1).to_bytes(4,'little')+(0x1725b0).to_bytes(4,'little')+bytes(4):
            raise ValueError('Invalid frozen container')
        saves.append(dict(started=p.started,finished=p.finished,thread=p.thread,result=p.result,
            frozen_sha256=hashlib.sha256(raw).hexdigest(),frozen_hex=raw.hex()))
    return dict(complete=True,authorizes_delivery=False,events=result,publications=saves)
