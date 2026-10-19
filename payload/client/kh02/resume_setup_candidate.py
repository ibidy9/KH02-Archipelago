"""Effects-off, exact AP record reload setup; fresh setup remains a different flag."""
import struct
from .fresh_setup_candidate import encode_setup as encode_fresh,MAX_SIZE,MAX_RESUME_RECORDS
from .receipt_set_candidate import decode

def encode_setup(*,key,history,supported,cookie,generation,journal_path,entries,target=1,publication_prefix=None):
    if target!=1 or not 1<=len(entries)<=MAX_RESUME_RECORDS:raise ValueError('Resume requires slot1 and one to 100 exact records')
    records=set()
    for record,receipt in entries:
        if len(record)!=0x3b40 or not any(record) or record in records:raise ValueError('Distinct nonempty records required')
        records.add(record);value=decode(receipt)
        if (value.key,value.owner_cookie,value.load_id)!=(key,cookie,1):raise ValueError('Reload receipt scope differs')
        if any(i.index>=len(history) or history[i.index]!=i or i.item_id not in supported for i in value.items):raise ValueError('Unreceived or unsupported membership')
    payload=bytearray(encode_fresh(key=key,history=history,supported=supported,cookie=cookie,generation=generation,journal_path=journal_path))
    if publication_prefix is not None:
        if len(publication_prefix)!=32 or any(ch not in '0123456789abcdef' for ch in publication_prefix) or int(publication_prefix,16)==0:raise ValueError('Unique hexadecimal publication prefix required')
        payload[-32:]=publication_prefix.encode('ascii')
    struct.pack_into('<I',payload,12,2);struct.pack_into('<II',payload,112,len(entries),target)
    for record,receipt in entries:payload+=struct.pack('<II',len(receipt),0)+record+receipt
    if len(payload)>MAX_SIZE:raise ValueError('Bounded reload setup capacity exceeded')
    struct.pack_into('<I',payload,8,len(payload));return bytes(payload)
