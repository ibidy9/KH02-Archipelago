/* Additive offline candidate; original receipt bindings remain unchanged. */
#include "kh02_receipt_bindings_candidate.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>
#define SET_CAP 64u /* unchanged bundle wire capacity */
#define HISTORY_SET_CAP (KH02_RECEIPT_MAX_ITEMS+1u)
/* Exact historical proofs grow on demand; never evict conflicting ownership evidence. */
#define BUNDLE_CAP 8u
typedef struct ReceiptCopy {uint64_t revision;unsigned char *bytes;uint32_t size;struct ReceiptCopy *next;} ReceiptCopy;
typedef struct RecordProof {struct RecordProof *next;uint64_t revision,hash;unsigned char bytes[KH02_BIND_RECORD];} RecordProof;
typedef struct HeldBundle {Kh02ReceiptBundle *data;Kh02BindingLease source;} HeldBundle;
struct Kh02ReceiptBindings {
    SRWLOCK lock;
    DWORD owner;
    volatile LONG revoked;
    uint64_t instance,serial,bank_instance,cookie,load_id;
    uint32_t team,slot,seed_size,set_count,proof_count;
    unsigned char seed[4096];
    Kh02BindingStore *records;
    uint64_t current[100];
    ReceiptCopy *sets;
    RecordProof *proofs;
    HeldBundle held[BUNDLE_CAP];
};
static volatile LONG64 instances;
#ifdef KH02_RECEIPT_BINDINGS_SELFTEST
static volatile LONG fault;
MEMBER_API void Kh02ReceiptBindingsTestFault(uint32_t value){InterlockedExchange(&fault,(LONG)value);}
#endif
_Static_assert(sizeof(Kh02ReceiptEntry)==16,"entry ABI");
_Static_assert(sizeof(Kh02ReceiptBundleLease)==24,"lease ABI");
_Static_assert(offsetof(Kh02ReceiptBundle,container)==1880,"container ABI");
static uint32_t u32(const unsigned char *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint64_t u64(const unsigned char *p){uint64_t n;memcpy(&n,p,8);return n;}
static uint32_t unlock(Kh02ReceiptBindings *s,uint32_t status){ReleaseSRWLockExclusive(&s->lock);return status;}
static uint32_t invalidate(Kh02ReceiptBindings *s) {
    InterlockedExchange(&s->revoked,1);return KH02_BIND_INVALID;
}
static uint32_t enter(Kh02ReceiptBindings *s,int owner,int allow_revoked) {
    if(!s)return KH02_BIND_REFUSED;
    if(owner && s->owner!=GetCurrentThreadId()){invalidate(s);return KH02_BIND_REFUSED;}
    if(!TryAcquireSRWLockExclusive(&s->lock)) {
        if(owner)invalidate(s);
        return KH02_BIND_BUSY;
    }
    if(s->revoked && !allow_revoked)return unlock(s,KH02_BIND_INVALID);
    return KH02_BIND_OK;
}
static unsigned char *copy_readonly(const unsigned char *data,uint32_t size,uint32_t point) {
    unsigned char *out;DWORD old;
#ifdef KH02_RECEIPT_BINDINGS_SELFTEST
    if(fault==(LONG)point)return NULL;
#else
    (void)point;
#endif
    out=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!out)return NULL;
    memcpy(out,data,size);
    if(!VirtualProtect(out,size,PAGE_READONLY,&old)){VirtualFree(out,0,MEM_RELEASE);return NULL;}
    return out;
}
Kh02ReceiptBindings *Kh02ReceiptBindingsCreate(const Kh02ReceiptScope *scope,const unsigned char *baseline,uint32_t size) {
    Kh02ReceiptBindings *s;LONG64 old,next;
    if(!scope || !scope->cookie || !scope->load_id || scope->reserved || !scope->seed ||
       !scope->seed_size || scope->seed_size>4096 ||
       !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)scope->seed,(int)scope->seed_size,NULL,0))return NULL;
    do {old=InterlockedCompareExchange64(&instances,0,0);if(old==LLONG_MAX)return NULL;next=old+1;}
    while(InterlockedCompareExchange64(&instances,next,old)!=old);
    s=VirtualAlloc(NULL,sizeof(*s),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!s)return NULL;
    s->records=Kh02BindingsCreate(scope->cookie,baseline,size);
    if(!s->records){VirtualFree(s,0,MEM_RELEASE);return NULL;}
    InitializeSRWLock(&s->lock);s->owner=GetCurrentThreadId();s->instance=(uint64_t)next;
    s->cookie=scope->cookie;s->load_id=scope->load_id;s->team=scope->team;s->slot=scope->slot;
    s->seed_size=scope->seed_size;memcpy(s->seed,scope->seed,scope->seed_size);return s;
}
static ReceiptCopy *find_set(Kh02ReceiptBindings *s,uint64_t revision) {
    ReceiptCopy *p;for(p=s->sets;p;p=p->next)if(p->revision==revision)return p;return NULL;
}
static int keep_set(Kh02ReceiptBindings *s,Kh02ReceiptOwner *owner,const Kh02ReceiptLease *lease) {
    unsigned char raw[KH02_RECEIPT_MAX_BYTES],*copy;uint32_t size=0;ReceiptCopy *existing,*node;
    if(!lease || !lease->instance || !lease->revision ||
       (s->bank_instance && s->bank_instance!=lease->instance) ||
       Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK ||
       Kh02ReceiptOwnerRead(owner,lease,raw,sizeof(raw),&size)!=KH02_BANK_OK ||
       Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK)return 0;
    /* OwnerRead validates lease and returns already parsed bank-owned bytes.
       Recheck complete scope before binding them into this independent store. */
    if(size<56+s->seed_size || u64(raw+16)!=s->cookie || u64(raw+24)!=s->load_id ||
       u64(raw+32)!=lease->revision || u32(raw+40)!=s->team || u32(raw+44)!=s->slot ||
       u32(raw+48)!=s->seed_size || memcmp(raw+56,s->seed,s->seed_size))return 0;
    existing=find_set(s,lease->revision);
    if(existing)return existing->size==size && !memcmp(existing->bytes,raw,size);
    if(s->set_count==HISTORY_SET_CAP || !(copy=copy_readonly(raw,size,1)))return 0;
    node=NULL;
#ifdef KH02_RECEIPT_BINDINGS_SELFTEST
    if(fault!=5)
#endif
    node=VirtualAlloc(NULL,sizeof(*node),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!node){VirtualFree(copy,0,MEM_RELEASE);return 0;}
    *node=(ReceiptCopy){lease->revision,copy,size,s->sets};s->sets=node;++s->set_count;
    s->bank_instance=lease->instance;return 1;
}
static int keep_proof(Kh02ReceiptBindings *s,uint64_t revision,const unsigned char *bytes) {
    RecordProof *p;DWORD old;uint64_t hash=14695981039346656037ULL;
    /* Hash only filters comparisons; equal hashes still require exact bytes. */
    for(uint32_t i=0;i<KH02_BIND_RECORD;i++)hash=(hash^bytes[i])*1099511628211ULL;
    for(p=s->proofs;p;p=p->next)if(p->hash==hash && !memcmp(p->bytes,bytes,KH02_BIND_RECORD))return p->revision==revision;
#ifdef KH02_RECEIPT_BINDINGS_SELFTEST
    if(fault==2)return 0;
#endif
    if(s->proof_count==UINT32_MAX)return 0;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return 0;
    p->next=s->proofs;p->revision=revision;p->hash=hash;memcpy(p->bytes,bytes,KH02_BIND_RECORD);
    if(!VirtualProtect(p,sizeof(*p),PAGE_READONLY,&old)){VirtualFree(p,0,MEM_RELEASE);return 0;}
    s->proofs=p;s->proof_count++;return 1;
}
uint32_t Kh02ReceiptBindingsPut(Kh02ReceiptBindings *s,uint32_t index,Kh02ReceiptOwner *owner,const Kh02ReceiptLease *lease,const unsigned char *bytes,uint32_t size) {
    uint32_t status=enter(s,1,0);if(status!=KH02_BIND_OK)return status;
    if(index>=100 || !bytes || size!=KH02_BIND_RECORD || !keep_set(s,owner,lease) || !keep_proof(s,lease->revision,bytes))
        return unlock(s,invalidate(s));
    status=Kh02BindingsPut(s->records,index,lease->revision,bytes,size);
    if(status!=KH02_BIND_OK)invalidate(s);else s->current[index]=lease->revision;
    return unlock(s,status);
}
uint32_t Kh02ReceiptBindingsRefresh(Kh02ReceiptBindings *s,uint32_t index,const unsigned char *before,const unsigned char *after,uint32_t size) {
    uint32_t status=enter(s,1,0);if(status!=KH02_BIND_OK)return status;
    if(index>=100 || !before || !after || size!=KH02_BIND_RECORD || !s->current[index] ||
       !keep_proof(s,s->current[index],after))return unlock(s,invalidate(s));
    status=Kh02BindingsRefresh(s->records,index,before,after,size);
    if(status!=KH02_BIND_OK)invalidate(s);return unlock(s,status);
}
static uint32_t release_source(Kh02ReceiptBindings *s,HeldBundle *held) {
    if(!held->source.data && !held->source.cookie && !held->source.serial)return KH02_BIND_OK;
    return Kh02BindingsRelease(s->records,&held->source);
}
uint32_t Kh02ReceiptBindingsCapture(Kh02ReceiptBindings *s,const unsigned char *raw,uint32_t size,Kh02ReceiptBundleLease *out) {
    const Kh02ReceiptBundleLease empty={0};Kh02ReceiptBundle *copy;HeldBundle *held;
    ReceiptCopy *selected[SET_CAP];uint32_t status,i,j,count=0,total=sizeof(*copy),offset,slot;DWORD old;
    if(!out || memcmp(out,&empty,sizeof(*out)))return KH02_BIND_REFUSED;
    status=enter(s,0,0);if(status!=KH02_BIND_OK)return status;
    for(slot=0;slot<BUNDLE_CAP;slot++)if(!s->held[slot].data && !s->held[slot].source.data)break;
    if(slot==BUNDLE_CAP || s->serial==UINT64_MAX)return unlock(s,KH02_BIND_CAPACITY);
    held=&s->held[slot];status=Kh02BindingsCapture(s->records,raw,size,&held->source);
    if(status!=KH02_BIND_OK)return unlock(s,status);
    total+=s->seed_size;
    for(i=0;i<100;i++) {
        uint64_t revision=held->source.data->revisions[i];ReceiptCopy *set;
        if(!revision)continue;
        for(j=0;j<count;j++)if(selected[j]->revision==revision)break;
        if(j<count)continue;
        set=find_set(s,revision);
        if(!set || count==SET_CAP)goto failed;
        selected[count++]=set;total+=set->size;
    }
#ifdef KH02_RECEIPT_BINDINGS_SELFTEST
    if(fault==3)goto failed;
#endif
    copy=VirtualAlloc(NULL,total,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copy)goto failed;
    copy->instance=s->instance;copy->serial=++s->serial;copy->cookie=s->cookie;copy->load_id=s->load_id;
    copy->size=total;copy->set_count=count;memcpy(copy->revisions,held->source.data->revisions,sizeof(copy->revisions));
    copy->team=s->team;copy->slot=s->slot;copy->seed_size=s->seed_size;copy->seed_offset=sizeof(*copy);
    memcpy((unsigned char*)copy+copy->seed_offset,s->seed,s->seed_size);
    memcpy(copy->container,held->source.data->container,KH02_BIND_SIZE);offset=sizeof(*copy)+s->seed_size;
    for(i=0;i<count;i++) {
        copy->sets[i]=(Kh02ReceiptEntry){selected[i]->revision,offset,selected[i]->size};
        memcpy((unsigned char*)copy+offset,selected[i]->bytes,selected[i]->size);offset+=selected[i]->size;
    }
    if(
#ifdef KH02_RECEIPT_BINDINGS_SELFTEST
       fault==4 ||
#endif
       !VirtualProtect(copy,total,PAGE_READONLY,&old)) {VirtualFree(copy,0,MEM_RELEASE);goto failed;}
    held->data=copy;*out=(Kh02ReceiptBundleLease){copy,s->instance,copy->serial};return unlock(s,KH02_BIND_OK);
failed:
    /* A busy release keeps its private lease in this slot for Destroy to drain.
       Never discard ownership just because construction of the envelope failed. */
    release_source(s,held);return unlock(s,KH02_BIND_INVALID);
}
uint32_t Kh02ReceiptBindingsRelease(Kh02ReceiptBindings *s,Kh02ReceiptBundleLease *out) {
    const Kh02ReceiptBundleLease empty={0};uint32_t status,i;
    if(!out)return KH02_BIND_REFUSED;
    status=enter(s,0,1);if(status!=KH02_BIND_OK)return status;
    if(!memcmp(out,&empty,sizeof(*out)))return unlock(s,KH02_BIND_OK);
    for(i=0;i<BUNDLE_CAP;i++)if(s->held[i].data==out->data && out->data &&
       out->instance==s->instance && out->serial==s->held[i].data->serial) {
        status=release_source(s,&s->held[i]);if(status!=KH02_BIND_OK)return unlock(s,status);
        if(!VirtualFree(s->held[i].data,0,MEM_RELEASE))return unlock(s,KH02_BIND_INVALID);
        s->held[i].data=NULL;memset(out,0,sizeof(*out));return unlock(s,KH02_BIND_OK);
    }
    return unlock(s,KH02_BIND_REFUSED);
}
uint32_t Kh02ReceiptBindingsInvalidate(Kh02ReceiptBindings *s) {
    if(!s)return KH02_BIND_REFUSED;invalidate(s);return KH02_BIND_OK;
}
uint32_t Kh02ReceiptBindingsDestroy(Kh02ReceiptBindings **handle) {
    Kh02ReceiptBindings *s;uint32_t status,i;
    if(!handle)return KH02_BIND_REFUSED;s=*handle;if(!s)return KH02_BIND_OK;
    status=enter(s,1,1);if(status!=KH02_BIND_OK)return status;invalidate(s);
    for(i=0;i<BUNDLE_CAP;i++)if(s->held[i].data)return unlock(s,KH02_BIND_BUSY);
    for(i=0;i<BUNDLE_CAP;i++)if((status=release_source(s,&s->held[i]))!=KH02_BIND_OK)return unlock(s,status);
    status=Kh02BindingsDestroy(&s->records);if(status!=KH02_BIND_OK)return unlock(s,status);
    while(s->sets) {
        ReceiptCopy *p=s->sets,*next=p->next;
        if(p->bytes && !VirtualFree(p->bytes,0,MEM_RELEASE))return unlock(s,KH02_BIND_INVALID);
        p->bytes=NULL;if(!VirtualFree(p,0,MEM_RELEASE))return unlock(s,KH02_BIND_INVALID);
        s->sets=next;--s->set_count;
    }
    while(s->proofs) {
        RecordProof *p=s->proofs,*next=p->next;
        if(!VirtualFree(p,0,MEM_RELEASE))return unlock(s,KH02_BIND_INVALID);
        s->proofs=next;
    }
    ReleaseSRWLockExclusive(&s->lock);
    if(!VirtualFree(s,0,MEM_RELEASE))return KH02_BIND_INVALID;*handle=NULL;return KH02_BIND_OK;
}

#include "kh02_save_header_bindings_candidate.h"
uint32_t Kh02ReceiptBindingsPutHeader(Kh02ReceiptBindings *s,uint32_t index,
    Kh02ReceiptOwner *owner,const Kh02ReceiptLease *lease,const unsigned char *bytes,uint32_t size,
    const unsigned char *header,uint32_t header_size) {
    uint32_t status=enter(s,1,0);if(status!=KH02_BIND_OK)return status;
    if(index>=100 || !bytes || size!=KH02_BIND_RECORD || !header || header_size!=0xb0 ||
       !keep_set(s,owner,lease) || !keep_proof(s,lease->revision,bytes))return unlock(s,invalidate(s));
    status=Kh02BindingsPutHeader(s->records,index,lease->revision,bytes,size,header,header_size);
    if(status!=KH02_BIND_OK)invalidate(s);else s->current[index]=lease->revision;
    return unlock(s,status);
}
