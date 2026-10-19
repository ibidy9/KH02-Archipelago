/* Additive bounded repeated-entry composition. Exact admission remains in Lifecycle.
   Caller authenticates and retains storage permit, baseline, lookup and service. */
#include "kh02_fresh_replay_delivery_candidate.c"
#include <stdio.h>
typedef unsigned (*Kh02ResumePermit)(void*);
typedef struct Kh02ResumeSaveEntry {
    Kh02SaveRouter *router;unsigned char *baseline;DWORD thread;
    Kh02ResumePermit permit;void *context;
    char prefix[65];uint32_t length,index;
    volatile LONG spent,flight,closed,busy;
} Kh02ResumeSaveEntry;
MEMBER_API Kh02ResumeSaveEntry *Kh02ResumeSaveEntryPrepare(Kh02SaveRouter *router,
    const unsigned char *raw,uint32_t size,const char *prefix,uint32_t length,
    uint32_t index,Kh02ResumePermit permit,void *context) {
    Kh02ResumeSaveEntry *p;Kh02ReceiptBindings *validation;DWORD old;
    if(!router || GetCurrentThreadId()!=router->thread || router_active(router) || router->count ||
       InterlockedCompareExchange(&router->closed,0,0) || !permit || !permit(context) ||
       !raw || size!=KH02_BIND_SIZE || index>=100 || !prefix || !length || length>40)return NULL;
    for(uint32_t i=0;i<length;i++)if(!((prefix[i]>='a'&&prefix[i]<='z') ||
       (prefix[i]>='A'&&prefix[i]<='Z') || (prefix[i]>='0'&&prefix[i]<='9') || prefix[i]=='-'))return NULL;
    validation=Kh02ReceiptBindingsCreate(&router->life->branch->scope,raw,size);
    if(!validation || Kh02ReceiptBindingsDestroy(&validation)!=KH02_BIND_OK)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->baseline=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!p->baseline){VirtualFree(p,0,MEM_RELEASE);return NULL;}
    memcpy(p->baseline,raw,size);
    if(!VirtualProtect(p->baseline,size,PAGE_READONLY,&old)) {
        VirtualFree(p->baseline,0,MEM_RELEASE);VirtualFree(p,0,MEM_RELEASE);return NULL;
    }
    p->router=router;p->thread=GetCurrentThreadId();p->permit=permit;p->context=context;
    p->index=index;p->length=length;memcpy(p->prefix,prefix,length);return p;
}
/* No SQLite or host wait here. Snapshot the already-known native container,
   and attach before returning to initialization/autosave. Each epoch gets an
   unrepeatable publication namespace within the sealed process preparation. */
static uint32_t session_attach(Kh02ResumeSaveEntry *p) {
    Kh02FrozenSave frozen={0};char prefix[65];uint32_t status=KH02_BIND_REFUSED;
    uint64_t epoch=Kh02LifecycleEpoch(p->router->life);
    if(!p->permit(p->context) || !Kh02LifecycleCurrent(p->router->life) || !epoch)return status;
    int length=snprintf(prefix,sizeof(prefix),"%s-%llu",p->prefix,(unsigned long long)epoch);
    if(length<=0 || length>64)return status;
    if(Kh02FreezeSave(p->router->config.read,p->router->config.read_context,p->router->config.base,
        p->router->config.base+0x4485520,&frozen)!=KH02_FREEZE_OK)return status;
    status=Kh02SaveRouterAttach(p->router,frozen.data,frozen.length,prefix,(uint32_t)length);
    if(Kh02ReleaseFrozenSave(&frozen)!=KH02_FREEZE_OK)status=KH02_BIND_REFUSED;
    return status;
}
static void session_failed(Kh02ResumeSaveEntry *p) {
    InterlockedExchange(&p->closed,1);Kh02SaveRouterStop(p->router);
}
MEMBER_API unsigned char Kh02ResumeSaveEntryLoad(Kh02ResumeSaveEntry *p,uintptr_t manager,uint32_t index) {
    unsigned char result;uint32_t eligible,entered=0;
    if(!p)return 0;InterlockedIncrement(&p->flight);
    eligible=GetCurrentThreadId()==p->thread && manager==p->router->config.base+0x4485500 &&
        !InterlockedCompareExchange(&p->closed,0,0) && p->permit(p->context);
    if(eligible)entered=InterlockedCompareExchange(&p->busy,1,0)==0;
    eligible=eligible && entered;
    if(!eligible)session_failed(p);
    if(eligible)InterlockedExchange(&p->spent,1);
    result=Kh02SaveRouterLoad(p->router,manager,index);
    if(!eligible)session_failed(p);
    else if(!result) {
        /* A native failed load retires ownership, but is not evidence of a
           permanent session fault. Keep publication detached. Only a later
           successful exact-record load can acquire a new owner. Never clear
           an existing close/revocation or revive unresolved old work. */
        if(!p->permit(p->context) || InterlockedCompareExchange(&p->router->closed,0,0) ||
           InterlockedCompareExchange(&p->router->life->closed,0,0) ||
           p->router->life->branch->result.status!=KH02_LOAD_FAILED ||
           router_active(p->router) || Kh02LifecycleCurrent(p->router->life))session_failed(p);
    } else if(session_attach(p)!=KH02_BIND_OK)session_failed(p);
    if(entered)InterlockedExchange(&p->busy,0);
    InterlockedDecrement(&p->flight);return result;
}
MEMBER_API uintptr_t Kh02SessionSaveEntryRestore(Kh02ResumeSaveEntry *p,uintptr_t context,uint32_t mode) {
    uintptr_t result;uint32_t eligible,entered=0;
    if(!p)return 0;InterlockedIncrement(&p->flight);
    eligible=GetCurrentThreadId()==p->thread && !InterlockedCompareExchange(&p->closed,0,0) && p->permit(p->context);
    if(eligible)entered=InterlockedCompareExchange(&p->busy,1,0)==0;
    eligible=eligible && entered && p->spent && Kh02LifecycleCurrent(p->router->life)!=NULL;
    if(!eligible)session_failed(p);
    result=Kh02SaveRouterRestore(p->router,context,mode);
    if(!eligible || session_attach(p)!=KH02_BIND_OK)session_failed(p);
    if(entered)InterlockedExchange(&p->busy,0);
    InterlockedDecrement(&p->flight);return result;
}
MEMBER_API uint32_t Kh02ResumeSaveEntryDestroy(Kh02ResumeSaveEntry **handle) {
    Kh02ResumeSaveEntry *p;if(!handle)return KH02_BIND_REFUSED;p=*handle;if(!p)return KH02_BIND_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BIND_REFUSED;
    InterlockedExchange(&p->closed,1);Kh02SaveRouterStop(p->router);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BIND_BUSY;
    if(p->baseline && !VirtualFree(p->baseline,0,MEM_RELEASE))return KH02_BIND_REFUSED;
    p->baseline=NULL;if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BIND_REFUSED;
    *handle=NULL;return KH02_BIND_OK;
}
