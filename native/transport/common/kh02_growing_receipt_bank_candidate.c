/* Additive bank: retain exact immutable historical revisions in individually
   allocated nodes. The same APIs, lease lifetimes, and validation rules apply.
   The defensive revision bound covers baseline plus the wire maximum items;
   exhaustion/allocation failure still occurs before any owner effect callback.
   No revision eviction or borrowed-buffer relocation is introduced. */
#include "kh02_receipt_bank_candidate.h"
#include <string.h>
#include <limits.h>
#define SET_CAP (KH02_RECEIPT_MAX_ITEMS+1u)
#define LEASE_CAP 64u
typedef struct BankSet {uint64_t revision,parent;unsigned char *data;uint32_t size,count;struct BankSet *next;} BankSet;
struct Kh02ReceiptBank {
    SRWLOCK lock;
    uint64_t instance,cookie,load_id,serial;
    DWORD owner;
    uint32_t closed,team,slot,seed_size,history_count,supported_count,set_count;
    unsigned char seed[4096];
    Kh02ReceiptIdentity history[KH02_RECEIPT_MAX_ITEMS];
    uint64_t supported[256];
    BankSet *first,*last;
    Kh02ReceiptLease leases[LEASE_CAP];
};
static volatile LONG64 instances;
#ifdef KH02_RECEIPT_BANK_SELFTEST
static volatile LONG test_fault;
KH02_RECEIPT_API void Kh02ReceiptBankTestFault(uint32_t value){InterlockedExchange(&test_fault,(LONG)value);}
#endif
_Static_assert(sizeof(Kh02ReceiptIdentity)==24,"receipt ABI");
_Static_assert(sizeof(Kh02ReceiptScope)==40,"scope ABI");
_Static_assert(sizeof(Kh02ReceiptLease)==24,"lease ABI");
static uint32_t u32(const unsigned char *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint64_t u64(const unsigned char *p){uint64_t n;memcpy(&n,p,8);return n;}
static int utf8(const unsigned char *s,uint32_t n){return s && n && n<=4096 && MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)s,(int)n,NULL,0)>0;}
static int history_ok(const Kh02ReceiptIdentity *items,uint32_t n) {
    uint32_t i;if(n>KH02_RECEIPT_MAX_ITEMS || (n && !items))return 0;
    for(i=0;i<n;i++)if(items[i].index!=i)return 0;
    return 1;
}
static uint32_t leave(Kh02ReceiptBank *b,uint32_t status){ReleaseSRWLockExclusive(&b->lock);return status;}
static uint32_t enter(Kh02ReceiptBank *b,int owner,int allow_closed) {
    if(!b || (owner && b->owner!=GetCurrentThreadId()))return KH02_BANK_REFUSED;
    if(!TryAcquireSRWLockExclusive(&b->lock))return KH02_BANK_BUSY;
    if(b->closed && !allow_closed)return leave(b,KH02_BANK_CLOSED);
    return KH02_BANK_OK;
}
static BankSet *find_set(Kh02ReceiptBank *b,uint64_t revision) {
    BankSet *set;for(set=b->first;set;set=set->next)if(set->revision==revision)return set;
    return NULL;
}
static int supported(Kh02ReceiptBank *b,uint64_t id) {
    uint32_t lo=0,hi=b->supported_count;
    while(lo<hi){uint32_t mid=lo+(hi-lo)/2;if(b->supported[mid]<id)lo=mid+1;else hi=mid;}
    return lo<b->supported_count && b->supported[lo]==id;
}
static int valid_set(Kh02ReceiptBank *b,const unsigned char *data,uint32_t size) {
    uint32_t n,seed,i;uint64_t previous=0;const unsigned char *p;
    if(!data || size<56 || size>KH02_RECEIPT_MAX_BYTES || memcmp(data,"KH02RSET",8) || u32(data+8)!=1 || u32(data+52))return 0;
    n=u32(data+12);seed=u32(data+48);
    if(n>KH02_RECEIPT_MAX_ITEMS || seed!=b->seed_size || size!=56+seed+24*n ||
       u64(data+16)!=b->cookie || u64(data+24)!=b->load_id || !u64(data+32) ||
       u32(data+40)!=b->team || u32(data+44)!=b->slot || memcmp(data+56,b->seed,seed))return 0;
    p=data+56+seed;
    for(i=0;i<n;i++,p+=24) {
        uint64_t index=u64(p);
        if(index>=b->history_count || (i && index<=previous) ||
           memcmp(p,&b->history[index],24) || !supported(b,u64(p+8)))return 0;
        previous=index;
    }
    return 1;
}
Kh02ReceiptBank *Kh02ReceiptBankCreate(const Kh02ReceiptScope *scope,const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t supported_count) {
    Kh02ReceiptBank *b;uint32_t i;LONG64 old,next;
    if(!scope || !scope->cookie || !scope->load_id || scope->reserved || !utf8(scope->seed,scope->seed_size) ||
       !history_ok(history,count) || supported_count>256 || (supported_count && !ids))return NULL;
    for(i=1;i<supported_count;i++)if(ids[i]<=ids[i-1])return NULL;
    do {old=InterlockedCompareExchange64(&instances,0,0);if(old==LLONG_MAX)return NULL;next=old+1;}
    while(InterlockedCompareExchange64(&instances,next,old)!=old);
    b=VirtualAlloc(NULL,sizeof(*b),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!b)return NULL;
    InitializeSRWLock(&b->lock);b->instance=(uint64_t)next;b->cookie=scope->cookie;b->load_id=scope->load_id;
    b->owner=GetCurrentThreadId();b->team=scope->team;b->slot=scope->slot;b->seed_size=scope->seed_size;
    memcpy(b->seed,scope->seed,scope->seed_size);b->history_count=count;b->supported_count=supported_count;
    if(count)memcpy(b->history,history,count*24);
    if(supported_count)memcpy(b->supported,ids,supported_count*8);
    return b;
}
uint32_t Kh02ReceiptBankHistory(Kh02ReceiptBank *b,const Kh02ReceiptIdentity *history,uint32_t count) {
    uint32_t status=enter(b,1,0);if(status!=KH02_BANK_OK)return status;
    if(!history_ok(history,count) || count<b->history_count || (b->history_count && memcmp(history,b->history,b->history_count*24)))return leave(b,KH02_BANK_REFUSED);
    if(count)memcpy(b->history,history,count*24);
    b->history_count=count;return leave(b,KH02_BANK_OK);
}
static uint32_t register_set(Kh02ReceiptBank *b,uint64_t parent,const unsigned char *data,uint32_t size) {
    uint32_t status=enter(b,1,0),i,j=0,n;uint64_t revision;BankSet *existing,*prior,*node;unsigned char *copy;DWORD old;
    if(status!=KH02_BANK_OK)return status;
    if(!valid_set(b,data,size))return leave(b,KH02_BANK_REFUSED);
    revision=u64(data+32);n=u32(data+12);existing=find_set(b,revision);
    if(existing)return leave(b,existing->parent==parent && existing->size==size && !memcmp(existing->data,data,size)?KH02_BANK_OK:KH02_BANK_REFUSED);
    if(!parent) {if(b->set_count)return leave(b,KH02_BANK_REFUSED);}
    else {
        const unsigned char *old_items,*new_items;
        prior=find_set(b,parent);
        if(!prior || revision<=parent || n!=prior->count+1)return leave(b,KH02_BANK_REFUSED);
        old_items=prior->data+56+b->seed_size;new_items=data+56+b->seed_size;
        for(i=0;i<n && j<prior->count;i++)if(!memcmp(new_items+24*i,old_items+24*j,24))j++;
        if(j!=prior->count)return leave(b,KH02_BANK_REFUSED);
    }
    if(b->set_count==SET_CAP)return leave(b,KH02_BANK_CAPACITY);
#ifdef KH02_RECEIPT_BANK_SELFTEST
    if(test_fault==1)return leave(b,KH02_BANK_CAPACITY);
#endif
    copy=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copy)return leave(b,KH02_BANK_CAPACITY);
    memcpy(copy,data,size);
    if(
#ifdef KH02_RECEIPT_BANK_SELFTEST
       test_fault==2 ||
#endif
       !VirtualProtect(copy,size,PAGE_READONLY,&old)) {
        VirtualFree(copy,0,MEM_RELEASE);return leave(b,KH02_BANK_REFUSED);
    }
    node=NULL;
#ifdef KH02_RECEIPT_BANK_SELFTEST
    if(test_fault!=3)
#endif
    node=VirtualAlloc(NULL,sizeof(*node),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!node){VirtualFree(copy,0,MEM_RELEASE);return leave(b,KH02_BANK_CAPACITY);}
    *node=(BankSet){revision,parent,copy,size,n,NULL};
    if(b->last)b->last->next=node;else b->first=node;
    b->last=node;++b->set_count;return leave(b,KH02_BANK_OK);
}
uint32_t Kh02ReceiptBankBaseline(Kh02ReceiptBank *b,const unsigned char *data,uint32_t size){return register_set(b,0,data,size);}
uint32_t Kh02ReceiptBankPropose(Kh02ReceiptBank *b,uint64_t parent,const unsigned char *data,uint32_t size) {
    return parent?register_set(b,parent,data,size):KH02_BANK_REFUSED;
}
uint32_t Kh02ReceiptBankAcquire(Kh02ReceiptBank *b,uint64_t revision,Kh02ReceiptLease *out) {
    const Kh02ReceiptLease empty={0};uint32_t status,i;
    if(!out || memcmp(out,&empty,sizeof(*out)))return KH02_BANK_REFUSED;
    status=enter(b,0,0);if(status!=KH02_BANK_OK)return status;
    if(!find_set(b,revision))return leave(b,KH02_BANK_REFUSED);
    for(i=0;i<LEASE_CAP;i++)if(!b->leases[i].serial)break;
    if(i==LEASE_CAP || b->serial==UINT64_MAX)return leave(b,KH02_BANK_CAPACITY);
    *out=b->leases[i]=(Kh02ReceiptLease){b->instance,++b->serial,revision};return leave(b,KH02_BANK_OK);
}
uint32_t Kh02ReceiptBankAdded(Kh02ReceiptBank *b,uint64_t parent,uint64_t revision,Kh02ReceiptIdentity *out) {
    uint32_t status,i,j=0;BankSet *before,*after;const unsigned char *p,*q;
    if(!out)return KH02_BANK_REFUSED;
    status=enter(b,1,0);if(status!=KH02_BANK_OK)return status;
    before=find_set(b,parent);after=find_set(b,revision);
    if(!before || !after || after->parent!=parent || after->count!=before->count+1)return leave(b,KH02_BANK_REFUSED);
    p=before->data+56+b->seed_size;q=after->data+56+b->seed_size;
    for(i=0;i<after->count;i++) {
        if(j<before->count && !memcmp(p+24*j,q+24*i,24)){j++;continue;}
        memcpy(out,q+24*i,24);return leave(b,KH02_BANK_OK);
    }
    return leave(b,KH02_BANK_REFUSED);
}
static int lease_index(Kh02ReceiptBank *b,const Kh02ReceiptLease *lease) {
    uint32_t i;if(!lease || !lease->serial || lease->instance!=b->instance)return -1;
    for(i=0;i<LEASE_CAP;i++)if(!memcmp(lease,&b->leases[i],sizeof(*lease)))return (int)i;
    return -1;
}
uint32_t Kh02ReceiptBankRead(Kh02ReceiptBank *b,const Kh02ReceiptLease *lease,unsigned char *out,uint32_t capacity,uint32_t *written) {
    uint32_t status;BankSet *set;
    if(!written)return KH02_BANK_REFUSED;*written=0;
    status=enter(b,0,1);if(status!=KH02_BANK_OK)return status;
    if(lease_index(b,lease)<0)return leave(b,KH02_BANK_REFUSED);
    set=find_set(b,lease->revision);
    if(!set || !out || capacity<set->size)return leave(b,KH02_BANK_REFUSED);
    memcpy(out,set->data,set->size);*written=set->size;return leave(b,KH02_BANK_OK);
}
uint32_t Kh02ReceiptBankRelease(Kh02ReceiptBank *b,Kh02ReceiptLease *lease) {
    const Kh02ReceiptLease empty={0};uint32_t status;int index;
    if(!lease)return KH02_BANK_REFUSED;
    status=enter(b,0,1);if(status!=KH02_BANK_OK)return status;
    if(!memcmp(lease,&empty,sizeof(*lease)))return leave(b,KH02_BANK_OK);
    index=lease_index(b,lease);if(index<0)return leave(b,KH02_BANK_REFUSED);
    memset(&b->leases[index],0,sizeof(*lease));memset(lease,0,sizeof(*lease));return leave(b,KH02_BANK_OK);
}
uint32_t Kh02ReceiptBankDestroy(Kh02ReceiptBank **owner) {
    Kh02ReceiptBank *b;uint32_t status,i;BankSet *node,*next;
    if(!owner)return KH02_BANK_REFUSED;b=*owner;if(!b)return KH02_BANK_OK;
    status=enter(b,1,1);if(status!=KH02_BANK_OK)return status;b->closed=1;
    for(i=0;i<LEASE_CAP;i++)if(b->leases[i].serial)return leave(b,KH02_BANK_BUSY);
    while((node=b->first)!=NULL) {
        if(node->data && !VirtualFree(node->data,0,MEM_RELEASE))return leave(b,KH02_BANK_REFUSED);
        node->data=NULL;next=node->next;
        if(!VirtualFree(node,0,MEM_RELEASE))return leave(b,KH02_BANK_REFUSED);
        b->first=next;--b->set_count;
    }
    b->last=NULL;
    /* Lifetime owner must already have joined every caller; leases are an
       additional guard, not protection against arbitrary use after destroy. */
    ReleaseSRWLockExclusive(&b->lock);
    if(!VirtualFree(b,0,MEM_RELEASE))return KH02_BANK_REFUSED;*owner=NULL;return KH02_BANK_OK;
}
