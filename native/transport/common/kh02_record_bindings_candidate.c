#include "kh02_record_bindings_candidate.h"
#include <string.h>
#if defined(KH02_CONSTRUCTED_PLATFORM_LIVE) && defined(KH02_BIND_SELFTEST)
#error Live release must not expose binding fault injection
#endif
#ifdef KH02_BIND_SELFTEST
static volatile LONG binding_test_fault;
__declspec(dllexport) void Kh02BindingsTestFault(uint32_t value){InterlockedExchange(&binding_test_fault,(LONG)value);}
#endif
typedef struct BindingRecord {uint64_t revision;unsigned char bytes[KH02_BIND_RECORD];} BindingRecord;
struct Kh02BindingStore {
    SRWLOCK lock;
    uint64_t cookie,serial;
    DWORD owner;
    volatile LONG revoked;
    unsigned char header[0xb0];
    BindingRecord records[100];
    const Kh02BindingBundle *held[8];
};
static int container_ok(const unsigned char *p,uint32_t size) {
    uint32_t fields[3];
    if(!p || size!=KH02_BIND_SIZE || memcmp(p,"SAVE",4))return 0;
    memcpy(fields,p+4,12);
    return fields[0]==1 && fields[1]==KH02_BIND_SIZE && !fields[2];
}
static uint32_t unlock(Kh02BindingStore *s,uint32_t result) {ReleaseSRWLockExclusive(&s->lock);return result;}
static int owned(Kh02BindingStore *s) {
    if(s->owner==GetCurrentThreadId())return 1;
    InterlockedExchange(&s->revoked,1);return 0;
}
Kh02BindingStore *Kh02BindingsCreate(uint64_t cookie,const unsigned char *baseline,uint32_t size) {
    Kh02BindingStore *s;uint32_t i;
    if(!cookie || !container_ok(baseline,size))return NULL;
    s=VirtualAlloc(NULL,sizeof(*s),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!s)return NULL;
    InitializeSRWLock(&s->lock);s->cookie=cookie;s->owner=GetCurrentThreadId();
    memcpy(s->header,baseline,0xb0);
    for(i=0;i<100;i++)memcpy(s->records[i].bytes,baseline+0xb0+i*KH02_BIND_RECORD,KH02_BIND_RECORD);
    return s;
}
uint32_t Kh02BindingsPut(Kh02BindingStore *s,uint32_t index,uint64_t revision,const unsigned char *bytes,uint32_t size) {
    uint32_t i;
    if(!s || !owned(s))return KH02_BIND_REFUSED;
    /* An observed failed write cannot leave an older binding eligible. */
    if(index>=100 || !revision || !bytes || size!=KH02_BIND_RECORD) {
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
    s->records[index].revision=revision;memcpy(s->records[index].bytes,bytes,size);
    return unlock(s,KH02_BIND_OK);
}
uint32_t Kh02BindingsRefresh(Kh02BindingStore *s,uint32_t index,const unsigned char *before,const unsigned char *after,uint32_t size) {
    uint32_t i;
    if(!s || !owned(s))return KH02_BIND_REFUSED;
    if(index>=100 || !before || !after || size!=KH02_BIND_RECORD) {
        InterlockedExchange(&s->revoked,1);return KH02_BIND_INVALID;
    }
    if(!TryAcquireSRWLockExclusive(&s->lock)) {
        InterlockedExchange(&s->revoked,1);return KH02_BIND_BUSY;
    }
    if(s->revoked)return unlock(s,KH02_BIND_INVALID);
    if(!s->records[index].revision || memcmp(s->records[index].bytes,before,size))goto bad;
    for(i=0;i<size;i++)if(before[i]!=after[i] &&
        !(i>=0x24 && i<0x30) && !(i>=0x3324 && i<0x3464))goto bad;
    for(i=0;i<100;i++)if(s->records[i].revision &&
        s->records[i].revision!=s->records[index].revision && !memcmp(s->records[i].bytes,after,size))goto bad;
    memcpy(s->records[index].bytes,after,size);return unlock(s,KH02_BIND_OK);
bad:
    InterlockedExchange(&s->revoked,1);return unlock(s,KH02_BIND_INVALID);
}
uint32_t Kh02BindingsCapture(Kh02BindingStore *s,const unsigned char *container,uint32_t size,Kh02BindingLease *out) {
    uint32_t i,slot=8;Kh02BindingBundle *copy;DWORD old;
    if(!s || !out || out->data || out->cookie || out->serial || !container_ok(container,size))return KH02_BIND_REFUSED;
    /* Try-only critical section: never wait for a Python host or game callback. */
    if(!TryAcquireSRWLockExclusive(&s->lock))return KH02_BIND_BUSY;
    if(s->revoked)return unlock(s,KH02_BIND_INVALID);
    if(memcmp(s->header,container,0xb0))return unlock(s,KH02_BIND_INVALID);
    for(i=0;i<100;i++)if(memcmp(s->records[i].bytes,container+0xb0+i*KH02_BIND_RECORD,KH02_BIND_RECORD))
        return unlock(s,KH02_BIND_INVALID);
    for(i=0;i<8;i++)if(!s->held[i]){slot=i;break;}
    if(slot==8 || s->serial==UINT64_MAX)return unlock(s,KH02_BIND_CAPACITY);
#ifdef KH02_BIND_SELFTEST
    if(binding_test_fault==1)return unlock(s,KH02_BIND_CAPACITY);
#endif
    copy=VirtualAlloc(NULL,sizeof(*copy),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!copy)return unlock(s,KH02_BIND_CAPACITY);
    copy->cookie=s->cookie;copy->serial=++s->serial;
    for(i=0;i<100;i++)copy->revisions[i]=s->records[i].revision;
    memcpy(copy->container,container,size);
    if(
#ifdef KH02_BIND_SELFTEST
       binding_test_fault==2 ||
#endif
       !VirtualProtect(copy,sizeof(*copy),PAGE_READONLY,&old)) {
        VirtualFree(copy,0,MEM_RELEASE);return unlock(s,KH02_BIND_INVALID);
    }
    s->held[slot]=copy;*out=(Kh02BindingLease){copy,s->cookie,copy->serial};return unlock(s,KH02_BIND_OK);
}
uint32_t Kh02BindingsRelease(Kh02BindingStore *s,Kh02BindingLease *out) {
    uint32_t i;
    if(!s || !out)return KH02_BIND_REFUSED;
    if(!out->data)return out->cookie || out->serial?KH02_BIND_REFUSED:KH02_BIND_OK;
#ifdef KH02_BIND_SELFTEST
    if(InterlockedCompareExchange(&binding_test_fault,0,3)==3)return KH02_BIND_BUSY;
#endif
    if(!TryAcquireSRWLockExclusive(&s->lock))return KH02_BIND_BUSY;
    for(i=0;i<8;i++)if(s->held[i]==out->data && out->cookie==s->cookie && out->serial==s->held[i]->serial) {
        if(!VirtualFree((void*)out->data,0,MEM_RELEASE))return unlock(s,KH02_BIND_INVALID);
        s->held[i]=NULL;memset(out,0,sizeof(*out));return unlock(s,KH02_BIND_OK);
    }
    return unlock(s,KH02_BIND_REFUSED);
}
uint32_t Kh02BindingsInvalidate(Kh02BindingStore *s) {
    if(!s)return KH02_BIND_REFUSED;
    InterlockedExchange(&s->revoked,1);return KH02_BIND_OK;
}
uint32_t Kh02BindingsDestroy(Kh02BindingStore **owner) {
    Kh02BindingStore *s;uint32_t i;
    if(!owner)return KH02_BIND_REFUSED;
    s=*owner;if(!s)return KH02_BIND_OK;
    if(!owned(s))return KH02_BIND_REFUSED;
    InterlockedExchange(&s->revoked,1);
    if(!TryAcquireSRWLockExclusive(&s->lock))return KH02_BIND_BUSY;
    for(i=0;i<8;i++)if(s->held[i])return unlock(s,KH02_BIND_BUSY);
    /* Caller must have joined all users. No API may race store destruction. */
    ReleaseSRWLockExclusive(&s->lock);
    if(!VirtualFree(s,0,MEM_RELEASE))return KH02_BIND_INVALID;
    *owner=NULL;return KH02_BIND_OK;
}
