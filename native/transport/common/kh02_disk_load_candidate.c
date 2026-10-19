#include "kh02_disk_load_candidate.h"
#include <string.h>
struct Kh02DiskLoad {
    Kh02DiskLoadConfig config;DWORD thread;
    volatile LONG closed,busy,flight;uint64_t generation,known_generation;
};
Kh02DiskLoad *Kh02DiskLoadCreate(const Kh02DiskLoadConfig *config) {
    Kh02DiskLoad *p;
    if(!config || !config->base || config->base>0x0000800000000000ULL-0x04f5f000 ||
       !config->cookie || !config->index_load_id || !config->read || !config->original || !config->lookup)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->config=*config;p->thread=GetCurrentThreadId();return p;
}
void Kh02DiskLoadStop(Kh02DiskLoad *p){if(p)InterlockedExchange(&p->closed,1);}
uint32_t Kh02DiskLoadCheck(Kh02DiskLoad *p,uint64_t generation) {
    return p && p->thread==GetCurrentThreadId() && generation &&
        !InterlockedCompareExchange(&p->closed,0,0) && !InterlockedCompareExchange(&p->busy,0,0) &&
        p->generation==generation && p->known_generation==generation?1:0;
}
unsigned char Kh02DiskLoadInvoke(Kh02DiskLoad *p,uintptr_t manager,uint32_t index,Kh02DiskLoadResult *out) {
    Kh02SaveSnapshot before,after;unsigned char result;uint32_t entered=0,valid=0,status=KH02_LOAD_CLOSED;
    if(!p)return 0;
    InterlockedIncrement(&p->flight);
    if(out){memset(out,0,sizeof(*out));out->index=index;}
    if(GetCurrentThreadId()!=p->thread){status=KH02_LOAD_WRONG_THREAD;Kh02DiskLoadStop(p);}
    else if(InterlockedCompareExchange(&p->busy,1,0)){status=KH02_LOAD_REENTRANT;Kh02DiskLoadStop(p);}
    else {
        entered=1;
        p->known_generation=0;
        /* Every owner-thread load attempt consumes a fresh identity BEFORE
           original execution, including unknown/failed loads. */
        if(p->generation==UINT64_MAX){status=KH02_LOAD_EXHAUSTED;Kh02DiskLoadStop(p);}
        else {
            ++p->generation;if(out)out->generation=p->generation;
            if(!InterlockedCompareExchange(&p->closed,0,0) && out) {
                valid=Kh02SaveSnapshotRead(p->config.read,p->config.read_context,p->config.base,manager,index,&before)==KH02_SNAPSHOT_OK;
                status=KH02_LOAD_BAD_SNAPSHOT;
            }
        }
    }
    /* Preserve native behavior even when observation is refused. */
    result=p->config.original(manager,index);
    if(entered && !InterlockedCompareExchange(&p->closed,0,0) && out) {
        if(!result)status=KH02_LOAD_FAILED;
        else if(valid && Kh02SaveSnapshotRead(p->config.read,p->config.read_context,p->config.base,manager,index,&after)==KH02_SNAPSHOT_OK &&
                !memcmp(before.record,after.record,KH02_SAVE_RECORD_SIZE)) {
            status=Kh02DiskLookupResolve(p->config.lookup,p->config.cookie,p->config.index_load_id,
                after.record,KH02_SAVE_RECORD_SIZE,out->receipts,sizeof(out->receipts),&out->receipt_size)==KH02_BANK_OK?
                KH02_LOAD_KNOWN:KH02_LOAD_UNKNOWN;
        }
    } else if(entered && status!=KH02_LOAD_EXHAUSTED)status=KH02_LOAD_CLOSED;
    if(entered && InterlockedCompareExchange(&p->closed,0,0) && status==KH02_LOAD_KNOWN)status=KH02_LOAD_CLOSED;
    if(entered && status==KH02_LOAD_KNOWN)p->known_generation=p->generation;
    if(out){out->status=status;if(status!=KH02_LOAD_KNOWN){out->receipt_size=0;memset(out->receipts,0,sizeof(out->receipts));}}
    if(entered)InterlockedExchange(&p->busy,0);
    InterlockedDecrement(&p->flight);return result;
}
uint32_t Kh02DiskLoadDestroy(Kh02DiskLoad **owner) {
    Kh02DiskLoad *p;
    if(!owner)return KH02_BANK_REFUSED;p=*owner;if(!p)return KH02_BANK_OK;
    if(p->thread!=GetCurrentThreadId())return KH02_BANK_REFUSED;
    Kh02DiskLoadStop(p);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BANK_BUSY;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BANK_BUSY;
    *owner=NULL;return KH02_BANK_OK;
}
