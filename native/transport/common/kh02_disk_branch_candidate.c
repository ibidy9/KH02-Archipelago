#include "kh02_disk_branch_candidate.h"
#include <string.h>
struct Kh02DiskBranch {
    Kh02DiskLoad *load;Kh02ReceiptScope scope;
    Kh02ReceiptIdentity history[4096];uint64_t supported[4096];unsigned char seed[4096];
    uint32_t history_count,supported_count,count;DWORD thread;
    volatile LONG busy,closed,flight;void *volatile active;uint64_t generation;
    Kh02ReceiptOwner *retained[64];Kh02DiskLoadResult result;
};
static void invalidate(Kh02DiskBranch *p) {
    Kh02ReceiptOwner *old=InterlockedExchangePointer(&p->active,NULL);
    if(old)Kh02ReceiptOwnerInvalidate(old);
}
static void close_branch(Kh02DiskBranch *p) {
    InterlockedExchange(&p->closed,1);invalidate(p);Kh02DiskLoadStop(p->load);
}
Kh02DiskBranch *Kh02DiskBranchCreate(const Kh02DiskLoadConfig *config,const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *supported,uint32_t supported_count) {
    Kh02ReceiptBank *validation;Kh02DiskBranch *p;
    if(!config || !scope || config->cookie!=scope->cookie || config->index_load_id!=scope->load_id ||
       scope->seed_size>4096 || count>4096 || supported_count>4096)return NULL;
    validation=Kh02ReceiptBankCreate(scope,history,count,supported,supported_count);
    if(!validation)return NULL;
    if(Kh02ReceiptBankDestroy(&validation)!=KH02_BANK_OK)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->scope=*scope;memcpy(p->seed,scope->seed,scope->seed_size);p->scope.seed=p->seed;
    if(count)memcpy(p->history,history,count*sizeof(*history));
    memcpy(p->supported,supported,supported_count*sizeof(*supported));
    p->history_count=count;p->supported_count=supported_count;p->thread=GetCurrentThreadId();
    p->load=Kh02DiskLoadCreate(config);
    if(!p->load){VirtualFree(p,0,MEM_RELEASE);return NULL;}return p;
}
unsigned char Kh02DiskBranchInvoke(Kh02DiskBranch *p,uintptr_t manager,uint32_t index) {
    unsigned char native;uint32_t entered=0;Kh02ReceiptOwner *owner=NULL;
    if(!p)return 0;
    InterlockedIncrement(&p->flight);
    /* Revoke the previous branch BEFORE original execution, including failure,
       wrong-thread and nested paths. Never resurrect an invalidated owner. */
    invalidate(p);
    if(GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,1,0))close_branch(p);
    else entered=1;
    native=Kh02DiskLoadInvoke(p->load,manager,index,entered?&p->result:NULL);
    if(entered && !InterlockedCompareExchange(&p->closed,0,0) && p->result.status==KH02_LOAD_KNOWN &&
       Kh02DiskLoadCheck(p->load,p->result.generation)) {
        if(p->count==64)close_branch(p);
        else {
            Kh02ReceiptScope scope=p->scope;scope.load_id=p->result.generation;
            /* The table's lifetime tag is not the active load identity. Change
               only that tag, then revalidate the whole set with the proven owner. */
            memcpy(p->result.receipts+24,&scope.load_id,8);
            owner=Kh02ReceiptOwnerCreate(&scope,p->history,p->history_count,p->supported,p->supported_count,
                p->result.receipts,p->result.receipt_size);
            if(!owner)close_branch(p);
            else {
                p->retained[p->count++]=owner;p->generation=p->result.generation;
                InterlockedExchangePointer(&p->active,owner);
                if(InterlockedCompareExchange(&p->closed,0,0) || !Kh02DiskLoadCheck(p->load,p->generation))invalidate(p);
            }
        }
    }
    if(entered)InterlockedExchange(&p->busy,0);
    InterlockedDecrement(&p->flight);return native;
}
Kh02ReceiptOwner *Kh02DiskBranchCurrent(Kh02DiskBranch *p) {
    Kh02ReceiptOwner *owner;
    if(!p || GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,0,0) ||
       InterlockedCompareExchange(&p->closed,0,0) || !Kh02DiskLoadCheck(p->load,p->generation))return NULL;
    owner=InterlockedCompareExchangePointer(&p->active,NULL,NULL);
    return owner && Kh02ReceiptOwnerCheck(owner)==KH02_MEMBER_OK?owner:NULL;
}
uint32_t Kh02DiskBranchDestroy(Kh02DiskBranch **handle) {
    Kh02DiskBranch *p;
    if(!handle)return KH02_BANK_REFUSED;p=*handle;if(!p)return KH02_BANK_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BANK_REFUSED;
    close_branch(p);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BANK_BUSY;
    /* Caller externally excludes/joins branch, owner and lease users before
       final destruction. Pending owner leases retain the containing branch. */
    for(uint32_t i=0;i<p->count;++i)
        if(Kh02ReceiptOwnerDestroy(&p->retained[i])!=KH02_BANK_OK)return KH02_BANK_BUSY;
    if(Kh02DiskLoadDestroy(&p->load)!=KH02_BANK_OK)return KH02_BANK_BUSY;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BANK_BUSY;
    *handle=NULL;return KH02_BANK_OK;
}
