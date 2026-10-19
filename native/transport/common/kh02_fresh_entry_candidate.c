#include "kh02_fresh_entry_candidate.h"
#include <string.h>
#include "kh02_opening_trace_v54.h"
void Kh02StartFreshCandidate(uintptr_t context);
struct Kh02FreshEntry {
    Kh02FreshConfig config;Kh02ReceiptOwner *owner;DWORD thread;
    volatile LONG spent,admitted,closed,flight,busy;
};
Kh02FreshEntry *Kh02FreshCreate(const Kh02FreshConfig *c,const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count,
    const unsigned char *empty,uint32_t size) {
    Kh02FreshEntry *p;uint32_t item_count;
    if(!c || !c->base || c->base>0x0000800000000000ULL-0x04f5f000 || !c->read || !c->original || !c->permit || !empty || size<56)return NULL;
    memcpy(&item_count,empty+12,4);if(item_count)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->config=*c;p->thread=GetCurrentThreadId();
    /* Existing owner fully validates framing, scope, history and supported IDs.
       It remains private until the paired initialization is proven. */
    p->owner=Kh02ReceiptOwnerCreate(scope,history,count,ids,id_count,empty,size);
    if(!p->owner){VirtualFree(p,0,MEM_RELEASE);return NULL;}return p;
}
void Kh02FreshRevoke(Kh02FreshEntry *p) {
    if(p){InterlockedExchange(&p->closed,1);InterlockedExchange(&p->admitted,0);Kh02ReceiptOwnerInvalidate(p->owner);}
}
uintptr_t Kh02FreshCopyInvoke(Kh02FreshEntry *p,uintptr_t caller,uintptr_t destination,uintptr_t source) {
    uintptr_t result,context=0;uint32_t entered=0,eligible=0;Kh02RestoreSnapshot before,after;
    if(!p)return 0;
    InterlockedIncrement(&p->flight);
    if(GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,1,0))Kh02FreshRevoke(p);
    else {
        entered=1;
        if(caller==p->config.base+0x56e277)Kh02FreshRevoke(p);
        if(caller==p->config.base+0x56e280) {
            if(InterlockedCompareExchange(&p->spent,1,0) || InterlockedCompareExchange(&p->closed,0,0))Kh02FreshRevoke(p);
            else if(source>=0x338 && source<0x0000800000000000ULL-0x4060) {
                context=source-0x338;
                eligible=destination==context+0x4060 && p->config.permit(p->config.permit_context)==1 &&
                    Kh02RestoreSnapshotRead(p->config.read,p->config.read_context,p->config.base,context,&before)==KH02_RESTORE_OK;
                if(!eligible)Kh02FreshRevoke(p);
            }else Kh02FreshRevoke(p);
        }
    }
    Kh02OpeningTraceCopy(1,p->config.base,caller,destination,source);
    result=p->config.original(destination,source);
    Kh02OpeningTraceCopy(2,p->config.base,caller,destination,source);
    if(eligible) {
        if(!InterlockedCompareExchange(&p->closed,0,0) && result==destination &&
           (!p->config.initialize || p->config.initialize(p->config.initialize_context,
                source,destination)==1) &&
           p->config.permit(p->config.permit_context)==1 &&
           Kh02RestoreSnapshotRead(p->config.read,p->config.read_context,p->config.base,context,&after)==KH02_RESTORE_OK &&
           !memcmp(before.primary,after.primary,sizeof(before.primary)) &&
           !memcmp(after.primary,after.checkpoint,sizeof(after.primary)) &&
           Kh02ReceiptOwnerCheck(p->owner)==KH02_MEMBER_OK) {
            InterlockedExchange(&p->admitted,1);
            if(InterlockedCompareExchange(&p->closed,0,0))InterlockedExchange(&p->admitted,0);
            else Kh02StartFreshCandidate(context);
        }else Kh02FreshRevoke(p);
    }
    if(entered)InterlockedExchange(&p->busy,0);
    InterlockedDecrement(&p->flight);return result;
}
Kh02ReceiptOwner *Kh02FreshCurrent(Kh02FreshEntry *p) {
    if(!p || GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,0,0) ||
       !InterlockedCompareExchange(&p->admitted,0,0) || InterlockedCompareExchange(&p->closed,0,0))return NULL;
    return Kh02ReceiptOwnerCheck(p->owner)==KH02_MEMBER_OK?p->owner:NULL;
}
uint32_t Kh02FreshDestroy(Kh02FreshEntry **handle) {
    Kh02FreshEntry *p;if(!handle)return KH02_BANK_REFUSED;p=*handle;if(!p)return KH02_BANK_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BANK_REFUSED;
    Kh02FreshRevoke(p);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BANK_BUSY;
    uint32_t status=Kh02ReceiptOwnerDestroy(&p->owner);if(status!=KH02_BANK_OK)return status;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BANK_BUSY;*handle=NULL;return KH02_BANK_OK;
}
