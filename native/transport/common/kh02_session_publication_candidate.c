/* Additive session publication: durable exact proofs outlive load owners. */
#include <windows.h>
#include <string.h>
#include "kh02_publication_gate_candidate.h"
#include "kh02_session_lookup_candidate.h"
enum { READY=0, WRITING=1, SAVING=2, DONE=3, REVOKED=16 };
struct Kh02PublicationGate {
    volatile LONG state;
    DWORD owner;
    Kh02NativeJournal *journal;
    const Kh02CanonicalAssociation *canonical;
    Kh02ReceiptBundle *session_bundle;
    Kh02PublicationOriginal original;
    uintptr_t args[4];
    Kh02PublicationService *service;
};
struct Kh02PublicationService {
    SRWLOCK lock;
    DWORD owner;
    volatile LONG closed;
    Kh02NativeJournal *journal;
    Kh02PublicationGate *attempts[64];
    Kh02DiskLookup *session_lookup; /* borrowed until all attempts join */
};
Kh02PublicationGate *Kh02PublicationCreate(const char *path,const unsigned char *bundle,uint32_t size,
    const unsigned char *token,uint32_t token_size,Kh02PublicationOriginal original,
    uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    Kh02PublicationGate *p;
    if(!path || !original)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!p)return NULL;
    p->owner=GetCurrentThreadId();p->original=original;
    p->args[0]=a;p->args[1]=b;p->args[2]=c;p->args[3]=d;
    p->canonical=Kh02CanonicalCreate(bundle,size,token,token_size);
    if(!p->canonical) {VirtualFree(p,0,MEM_RELEASE);return NULL;}
    p->journal=Kh02NativeJournalOpen(path);
    if(!p->journal) {Kh02CanonicalDestroy(&p->canonical);VirtualFree(p,0,MEM_RELEASE);return NULL;}
    return p;
}
static uint32_t finish(Kh02PublicationGate *p,uint32_t result) {
    LONG old;
    do {old=InterlockedCompareExchange(&p->state,0,0);}
    while(InterlockedCompareExchange(&p->state,DONE|(old&REVOKED),old)!=old);
    /* No owner access after DONE: joined-caller destruction still required. */
    return result;
}
uint32_t Kh02PublicationExecute(Kh02PublicationGate *p) {
    char digest[65],expected[65];uint32_t i,status;LONG previous;
    Kh02FrozenSave descriptor;
    static const char hex[]="0123456789abcdef";
    if(!p)return KH02_PUBLICATION_CANCELLED;
    previous=InterlockedCompareExchange(&p->state,WRITING,READY);
    if(previous!=READY)return previous==REVOKED?KH02_PUBLICATION_CANCELLED:KH02_PUBLICATION_ALREADY_USED;
    for(i=0;i<32;i++) {expected[2*i]=hex[p->canonical->binding[i]>>4];expected[2*i+1]=hex[p->canonical->binding[i]&15];}
    expected[64]=0;
    status=Kh02NativeJournalAppend(p->journal,p->canonical,digest);
    if(status!=KH02_JOURNAL_DURABLE || memcmp(digest,expected,65))return finish(p,KH02_PUBLICATION_JOURNAL_FAILED);
    /* This runs only after durable append and before native publication. The
       canonical validator already checked all offsets and record/set matches.
       A failed/contended proof append suppresses publication; never guess empty.
       Retained proof may describe an unused durable association, exactly as the
       journal can after cancellation. Native exact-load evidence is still required. */
    if(p->session_bundle) {
        const Kh02ReceiptBundle *bundle=p->session_bundle;
        for(uint32_t record=0;record<100;record++)if(bundle->revisions[record]) {
            uint32_t set=0;
            for(;set<bundle->set_count;set++)if(bundle->sets[set].revision==bundle->revisions[record])break;
            if(set==bundle->set_count || Kh02SessionLookupRemember(p->service->session_lookup,bundle->load_id,
                bundle->container+0xb0+record*0x3b40,0x3b40,
                (const unsigned char*)bundle+bundle->sets[set].offset,bundle->sets[set].size)!=KH02_BANK_OK)
                return finish(p,KH02_PUBLICATION_JOURNAL_FAILED);
        }
    }
    /* Service close and save admission share a short try-lock. Release BEFORE
       original; neither journal I/O nor the original runs under this lock. */
    if(p->service) {
        Kh02PublicationService *s=p->service;
        if(!TryAcquireSRWLockExclusive(&s->lock))return finish(p,KH02_PUBLICATION_CANCELLED);
        previous=s->closed?REVOKED:InterlockedCompareExchange(&p->state,SAVING,WRITING);
        ReleaseSRWLockExclusive(&s->lock);
    } else previous=InterlockedCompareExchange(&p->state,SAVING,WRITING);
    if(previous!=WRITING)return finish(p,KH02_PUBLICATION_CANCELLED);
    descriptor.data=(const unsigned char*)p->canonical+p->canonical->container.offset;
    descriptor.length=descriptor.capacity=KH02_FULL_SAVE_SIZE;
    status=p->original(p->args[0],p->args[1],p->args[2],p->args[3],(uintptr_t)&descriptor);
    return finish(p,status?KH02_PUBLICATION_SAVED:KH02_PUBLICATION_SAVE_FAILED);
}
void Kh02PublicationRevoke(Kh02PublicationGate *p) {if(p)InterlockedOr(&p->state,REVOKED);}
uint32_t Kh02PublicationClose(Kh02PublicationGate **owner) {
    Kh02PublicationGate *p;Kh02PublicationService *service;LONG state;uint32_t status,i;
    if(!owner)return KH02_JOURNAL_REFUSED;
    p=*owner;if(!p)return KH02_JOURNAL_DURABLE;
    if(p->owner!=GetCurrentThreadId())return KH02_JOURNAL_REFUSED;
    state=InterlockedOr(&p->state,REVOKED)&~REVOKED;
    if(state==WRITING || state==SAVING)return KH02_JOURNAL_BUSY;
    service=p->service;
    if(service) {
        if(!TryAcquireSRWLockExclusive(&service->lock))return KH02_JOURNAL_BUSY;
        if(p->canonical && !Kh02CanonicalDestroy(&p->canonical)) {
            ReleaseSRWLockExclusive(&service->lock);return KH02_JOURNAL_FAILED;
        }
        for(i=0;i<64;i++)if(service->attempts[i]==p)service->attempts[i]=NULL;
        /* The connection belongs to service, which must outlive this owner. */
        p->service=NULL;p->journal=NULL;
        ReleaseSRWLockExclusive(&service->lock);
    } else {
        status=Kh02NativeJournalClose(&p->journal);
        if(status!=KH02_JOURNAL_DURABLE)return status;
        if(p->canonical && !Kh02CanonicalDestroy(&p->canonical))return KH02_JOURNAL_FAILED;
    }
    if(p->session_bundle && !VirtualFree(p->session_bundle,0,MEM_RELEASE))return KH02_JOURNAL_FAILED;
    p->session_bundle=NULL;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_JOURNAL_FAILED;
    *owner=NULL;return KH02_JOURNAL_DURABLE;
}
Kh02PublicationService *Kh02PublicationServiceOpen(const char *path) {
    Kh02PublicationService *s;
    if(!path)return NULL;
    s=VirtualAlloc(NULL,sizeof(*s),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!s)return NULL;
    s->owner=GetCurrentThreadId();s->journal=Kh02NativeJournalOpen(path);
    if(!s->journal){VirtualFree(s,0,MEM_RELEASE);return NULL;}
    return s;
}
Kh02PublicationGate *Kh02PublicationCreateAttempt(Kh02PublicationService *s,
    const unsigned char *bundle,uint32_t size,const unsigned char *token,uint32_t token_size,
    Kh02PublicationOriginal original,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    Kh02PublicationGate *p;uint32_t i;
    if(!s || !original || InterlockedCompareExchange(&s->closed,0,0))return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!p)return NULL;
    p->canonical=Kh02CanonicalCreate(bundle,size,token,token_size);
    if(!p->canonical) {VirtualFree(p,0,MEM_RELEASE);return NULL;}
    if(s->session_lookup) {
        DWORD old;
        p->session_bundle=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        if(!p->session_bundle)goto refused;
        memcpy(p->session_bundle,bundle,size);
        if(!VirtualProtect(p->session_bundle,size,PAGE_READONLY,&old))goto refused;
    }
    if(!TryAcquireSRWLockExclusive(&s->lock))goto refused;
    if(s->closed){ReleaseSRWLockExclusive(&s->lock);goto refused;}
    for(i=0;i<64;i++)if(!s->attempts[i])break;
    if(i==64){ReleaseSRWLockExclusive(&s->lock);goto refused;}
    p->owner=s->owner;p->original=original;p->journal=s->journal;p->service=s;
    p->args[0]=a;p->args[1]=b;p->args[2]=c;p->args[3]=d;s->attempts[i]=p;
    ReleaseSRWLockExclusive(&s->lock);return p;
refused:
    if(p->session_bundle)VirtualFree(p->session_bundle,0,MEM_RELEASE);
    Kh02CanonicalDestroy(&p->canonical);VirtualFree(p,0,MEM_RELEASE);return NULL;
}
uint32_t Kh02PublicationServiceClose(Kh02PublicationService **owner) {
    Kh02PublicationService *s;uint32_t i,status;int held=0;
    if(!owner)return KH02_JOURNAL_REFUSED;
    s=*owner;if(!s)return KH02_JOURNAL_DURABLE;
    if(s->owner!=GetCurrentThreadId())return KH02_JOURNAL_REFUSED;
    if(!TryAcquireSRWLockExclusive(&s->lock))return KH02_JOURNAL_BUSY;
    InterlockedExchange(&s->closed,1);
    for(i=0;i<64;i++)if(s->attempts[i]) {Kh02PublicationRevoke(s->attempts[i]);held=1;}
    ReleaseSRWLockExclusive(&s->lock);
    if(held)return KH02_JOURNAL_BUSY;
    status=Kh02NativeJournalClose(&s->journal);
    if(status!=KH02_JOURNAL_DURABLE)return status;
    if(!VirtualFree(s,0,MEM_RELEASE))return KH02_JOURNAL_FAILED;
    *owner=NULL;return KH02_JOURNAL_DURABLE;
}

/* Creating thread prepares this before exposing it to any callback. There is
   no API to rebind a live service to another seed/lookup. */
MEMBER_API Kh02PublicationService *Kh02SessionPublicationServiceOpen(const char *path,Kh02DiskLookup *lookup) {
    Kh02PublicationService *s;if(!lookup)return NULL;
    s=Kh02PublicationServiceOpen(path);if(s)s->session_lookup=lookup;return s;
}
