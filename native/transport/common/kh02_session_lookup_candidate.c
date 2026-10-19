#include "resume_bounds_v254.h"
/* Offline candidate: process-owned exact evidence, independent of load owners.
   Existing immutable lookup and its working binaries remain unchanged. */
#include "kh02_session_lookup_candidate.h"
#include <string.h>
typedef struct SessionProof {
    struct SessionProof *next;
    uint64_t hash;
    uint32_t receipt_size;
    unsigned char bytes[]; /* exact record, then normalized receipt envelope */
} SessionProof;
struct Kh02DiskLookup {
    SRWLOCK lock; DWORD thread; volatile LONG closed;
    Kh02ReceiptScope scope;
    unsigned char seed[4096];
    Kh02ReceiptIdentity history[KH02_RECEIPT_MAX_ITEMS];
    uint64_t supported[256];
    uint32_t history_count,supported_count;
    uint64_t count,bytes;
    SessionProof *proofs;
};
static uint64_t record_hash(const unsigned char *p) {
    uint64_t h=14695981039346656037ULL;
    for(uint32_t i=0;i<0x3b40;i++)h=(h^p[i])*1099511628211ULL;
    return h;
}
static uint32_t leave(Kh02DiskLookup *p,uint32_t status) {
    ReleaseSRWLockExclusive(&p->lock);return status;
}
uint32_t Kh02SessionLookupRemember(Kh02DiskLookup *p,uint64_t epoch,
    const unsigned char *record,uint32_t record_size,const unsigned char *receipts,uint32_t size) {
    Kh02ReceiptBank *bank=NULL;Kh02ReceiptScope source;
    SessionProof *node=NULL,*old;uint64_t hash,one=1;DWORD protection;
    uint32_t status=KH02_BANK_REFUSED;
    if(!p || !epoch || !record || record_size!=0x3b40 || !receipts ||
       size<56 || size>KH02_RECEIPT_MAX_BYTES)return status;
    if(!TryAcquireSRWLockExclusive(&p->lock))return KH02_BANK_BUSY;
    if(p->closed)return leave(p,KH02_BANK_CLOSED);
    source=p->scope;source.load_id=epoch;
    bank=Kh02ReceiptBankCreate(&source,p->history,p->history_count,p->supported,p->supported_count);
    if(!bank)goto failed;
    if(Kh02ReceiptBankBaseline(bank,receipts,size)!=KH02_BANK_OK)goto failed;
    if(Kh02ReceiptBankDestroy(&bank)!=KH02_BANK_OK)goto failed;
    node=VirtualAlloc(NULL,sizeof(*node)+0x3b40+size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!node){status=KH02_BANK_CAPACITY;goto failed;}
    memcpy(node->bytes,record,0x3b40);memcpy(node->bytes+0x3b40,receipts,size);
    memcpy(node->bytes+0x3b40+24,&p->scope.load_id,8);
    memcpy(node->bytes+0x3b40+32,&one,8);
    hash=record_hash(record);
    for(old=p->proofs;old;old=old->next)if(old->hash==hash && !memcmp(old->bytes,record,0x3b40)) {
        status=old->receipt_size==size && !memcmp(old->bytes+0x3b40,node->bytes+0x3b40,size)?
            KH02_BANK_OK:KH02_BANK_REFUSED;
        VirtualFree(node,0,MEM_RELEASE);node=NULL;
        if(status!=KH02_BANK_OK)goto failed;
        return leave(p,status);
    }
    if(p->count==UINT64_MAX || UINT64_MAX-p->bytes<sizeof(*node)+0x3b40+size) {
        status=KH02_BANK_CAPACITY;goto failed;
    }
    node->next=p->proofs;node->hash=hash;node->receipt_size=size;
    if(!VirtualProtect(node,sizeof(*node)+0x3b40+size,PAGE_READONLY,&protection))goto failed;
    p->proofs=node;p->count++;p->bytes+=sizeof(*node)+0x3b40+size;
    return leave(p,KH02_BANK_OK);
failed:
    if(bank)Kh02ReceiptBankDestroy(&bank);
    if(node)VirtualFree(node,0,MEM_RELEASE);
    InterlockedExchange(&p->closed,1);return leave(p,status);
}
static Kh02DiskLookup *create_empty(const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count) {
    Kh02ReceiptBank *bank=Kh02ReceiptBankCreate(scope,history,count,ids,id_count);
    Kh02DiskLookup *p;
    if(!bank)return NULL;
    if(Kh02ReceiptBankDestroy(&bank)!=KH02_BANK_OK)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->thread=GetCurrentThreadId();p->scope=*scope;
    memcpy(p->seed,scope->seed,scope->seed_size);p->scope.seed=p->seed;
    p->history_count=count;p->supported_count=id_count;
    if(count)memcpy(p->history,history,count*sizeof(*history));
    if(id_count)memcpy(p->supported,ids,id_count*sizeof(*ids));
    InitializeSRWLock(&p->lock);return p;
}
Kh02DiskLookup *Kh02DiskLookupCreate(const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count,
    const Kh02DiskEntry *entries,uint32_t entry_count) {
    Kh02DiskLookup *p;uint64_t revision;
    /* Startup wire retains its existing limit. Growth happens only via trusted
       publication evidence; this does not widen the external setup contract. */
    if(!entries || !entry_count || entry_count>KH02_RESUME_RECORDS_V254)return NULL;
    p=create_empty(scope,history,count,ids,id_count);if(!p)return NULL;
    for(uint32_t i=0;i<entry_count;i++) {
        const Kh02DiskEntry *e=&entries[i];
        if(e->reserved || e->reserved2 || !e->receipts || e->receipt_size<56)goto failed;
        memcpy(&revision,e->receipts+32,8);if(revision!=1)goto failed;
        if(Kh02SessionLookupRemember(p,scope->load_id,e->record,e->record_size,e->receipts,e->receipt_size)!=KH02_BANK_OK)goto failed;
    }
    return p;
failed:Kh02DiskLookupDestroy(&p);return NULL;
}
uint32_t Kh02DiskLookupResolve(Kh02DiskLookup *p,uint64_t cookie,uint64_t load_id,
    const unsigned char *record,uint32_t size,unsigned char *out,uint32_t capacity,uint32_t *written) {
    uint32_t status=KH02_BANK_REFUSED;uint64_t hash;SessionProof *node;
    if(written)*written=0;
    if(!p || !record || size!=0x3b40 || !out || !written ||
       cookie!=p->scope.cookie || load_id!=p->scope.load_id)return status;
    if(!TryAcquireSRWLockShared(&p->lock))return KH02_BANK_BUSY;
    if(p->closed){ReleaseSRWLockShared(&p->lock);return KH02_BANK_CLOSED;}
    hash=record_hash(record);
    for(node=p->proofs;node;node=node->next)if(node->hash==hash && !memcmp(node->bytes,record,size)) {
        if(capacity<node->receipt_size)status=KH02_BANK_CAPACITY;
        else {memcpy(out,node->bytes+size,node->receipt_size);*written=node->receipt_size;status=KH02_BANK_OK;}
        break;
    }
    ReleaseSRWLockShared(&p->lock);return status;
}
uint32_t Kh02DiskLookupDestroy(Kh02DiskLookup **owner) {
    Kh02DiskLookup *p;SessionProof *node,*next;
    if(!owner)return KH02_BANK_REFUSED;p=*owner;if(!p)return KH02_BANK_OK;
    if(p->thread!=GetCurrentThreadId())return KH02_BANK_REFUSED;
    InterlockedExchange(&p->closed,1);
    if(!TryAcquireSRWLockExclusive(&p->lock))return KH02_BANK_BUSY;
    while((node=p->proofs)) {
        next=node->next;
        if(!VirtualFree(node,0,MEM_RELEASE))return leave(p,KH02_BANK_BUSY);
        p->proofs=next;
    }
    /* External exclusion/join remains mandatory; a free lock is not a join. */
    ReleaseSRWLockExclusive(&p->lock);
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BANK_BUSY;
    *owner=NULL;return KH02_BANK_OK;
}
