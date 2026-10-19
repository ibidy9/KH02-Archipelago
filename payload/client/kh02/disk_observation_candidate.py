"""Bounded effects-off disk observation wire and post-exit evidence parser."""
import ctypes as c
import struct
from .route_transport_candidate import RouteBlock,MAGIC,VERSION
from .receipt_set_candidate import decode,validate

HEADER=struct.Struct('<8I2Q2IQ')
MAX_SETUP=64+4096+4096*32+64*(8+0x3b40+102456)
MAX_EVENTS=16

def encode_setup(inputs):
    seed=inputs.key.seed_name.encode('utf-8')
    if not 0<len(seed)<=4096 or not 0<len(inputs.entries)<=64:
        raise ValueError('Setup capacity exceeded')
    if len(inputs.history)>4096 or not 0<len(inputs.supported)<=4096:
        raise ValueError('History/support capacity exceeded')
    if tuple(item.index for item in inputs.history)!=tuple(range(len(inputs.history))):
        raise ValueError('Complete original history required')
    if len(set(inputs.supported))!=len(inputs.supported):raise ValueError('Duplicate supported item')
    parts=[seed]
    parts.extend(struct.pack('<QQQ',i.index,i.item_id,i.player) for i in inputs.history)
    parts.extend(struct.pack('<Q',i) for i in inputs.supported)
    seen=set()
    for record,receipts in inputs.entries:
        if type(record) is not bytes or len(record)!=0x3b40 or record in seen:
            raise ValueError('Exact unique records required')
        seen.add(record)
        value=decode(receipts)
        validate(value,inputs.key,inputs.cookie,inputs.load_id,inputs.history,inputs.supported)
        if value.revision!=1:raise ValueError('Immutable index revision must be one')
        parts.extend((struct.pack('<II',len(receipts),0),record,receipts))
    body=b''.join(parts)
    return HEADER.pack(0x4b444c53,1,64+len(body),len(seed),len(inputs.history),len(inputs.supported),
        len(inputs.entries),0,inputs.cookie,inputs.load_id,inputs.key.team,inputs.key.slot,0)+body

class LoadResult(c.Structure):
    _fields_=[('generation',c.c_uint64)]+[(n,c.c_uint32) for n in ('status','index','receipt_size','reserved')]+[('receipts',c.c_ubyte*102456)]
class LoadEvent(c.Structure):
    _fields_=[('started',c.c_uint64),('finished',c.c_uint64)]+[(n,c.c_uint32) for n in ('thread','native_result','published','reserved')]+[('evidence',LoadResult)]
class ObservationBlock(RouteBlock):
    _fields_=[('setup_size',c.c_uint32),('reserved_setup',c.c_uint32),('setup',c.c_ubyte*MAX_SETUP),
              ('count',c.c_int32),('overflow',c.c_int32),('events',LoadEvent*MAX_EVENTS)]

def parse_closed(block):
    """Caller must first establish original-process exit; never read live payloads."""
    if (block.magic,block.version,block.size)!=(MAGIC,VERSION,c.sizeof(ObservationBlock)):
        raise ValueError('Disk observation ABI mismatch')
    complete=0<block.count<=MAX_EVENTS and block.overflow==0
    events=[]
    for event in block.events[:max(0,min(block.count,MAX_EVENTS))]:
        r=event.evidence
        valid=(event.published==1 and event.finished>=event.started and event.thread!=0 and
               event.native_result in (0,1) and not event.reserved and not r.reserved and
               r.status in range(1,9) and r.receipt_size<=102456)
        receipts=None
        if valid and r.status==1:
            try:
                receipts=decode(bytes(r.receipts[:r.receipt_size]))
                valid=bool(r.generation and event.native_result==1)
            except ValueError:valid=False
        elif r.receipt_size or any(r.receipts):valid=False
        complete=complete and valid
        events.append(dict(started=event.started,finished=event.finished,thread=event.thread,
            native_result=event.native_result,generation=r.generation,status=r.status,index=r.index,
            complete=bool(valid),receipts_hex=bytes(r.receipts[:r.receipt_size]).hex() if valid and receipts else None))
    return dict(authorizes_delivery=False,complete=bool(complete),count=block.count,overflow=block.overflow,events=events)
