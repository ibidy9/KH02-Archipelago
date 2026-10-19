#include "kh02_record_bindings_candidate.c"
#include "kh02_save_header_bindings_candidate.h"
uint32_t Kh02BindingsPutHeader(Kh02BindingStore *s,uint32_t index,uint64_t revision,
    const unsigned char *bytes,uint32_t size,const unsigned char *header,uint32_t header_size) {
    uint32_t i;
    if(!s || !owned(s))return KH02_BIND_REFUSED;
    if(index>=100 || !revision || !bytes || size!=KH02_BIND_RECORD || !header || header_size!=0xb0 ||
       memcmp(header,"SAVE",4) || memcmp(header+4,s->header+4,12)) {
        InterlockedExchange(&s->revoked,1);return KH02_BIND_INVALID;
    }
    if(!TryAcquireSRWLockExclusive(&s->lock)) {
        InterlockedExchange(&s->revoked,1);return KH02_BIND_BUSY;
    }
    if(s->revoked)return unlock(s,KH02_BIND_INVALID);
    for(i=0;i<100;i++)if(s->records[i].revision && s->records[i].revision!=revision &&
        !memcmp(s->records[i].bytes,bytes,size)) {
        InterlockedExchange(&s->revoked,1);return unlock(s,KH02_BIND_INVALID);
    }
    /* A capture sees both new values or neither under the same lock. */
    s->records[index].revision=revision;memcpy(s->records[index].bytes,bytes,size);
    memcpy(s->header,header,header_size);return unlock(s,KH02_BIND_OK);
}
