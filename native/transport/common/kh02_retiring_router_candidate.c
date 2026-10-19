/* Additive effects-off composition. All graph borrowers must enter this router.
   Raw asynchronous owner borrowing and the fresh-entry lane are not supported.
   Outer object destruction still requires external hook exclusion/join. */
#include "kh02_save_lifecycle_candidate.c"
#include "kh02_callback_domain_candidate.c"
#include <stdio.h>
#ifdef KH02_FRESH_LIFECYCLE
#error Retiring router requires an audited fresh-owner borrow contract before fresh entry is enabled
#endif
typedef struct Kh02SaveRouter {
    Kh02Lifecycle *life;Kh02SavePipelineConfig config;
    Kh02SavePipeline *held[8];uint32_t count;
    DWORD thread;volatile LONG closed;Kh02CallbackDomain domain;
    char namespace_root[41];uint64_t attached_epoch;
} Kh02SaveRouter;
#ifdef KH02_FRESH_SETUP_SELFTEST
static void (*router_test_selected)(uint32_t,void*);
MEMBER_API void Kh02RetiringRouterTestSelect(void (*callback)(uint32_t,void*)) {router_test_selected=callback;}
MEMBER_API uint32_t Kh02RetiringRouterTestDrained(Kh02SavePipeline *p) {return p->drained;}
#define SELECTED(kind,pin) do {if(router_test_selected)router_test_selected(kind,(pin).selected);} while(0)
#else
#define SELECTED(kind,pin) ((void)0)
#endif
static uint32_t router_flight(Kh02SaveRouter *p) {
    uint32_t count;AcquireSRWLockExclusive(&p->domain.gate);
    count=p->domain.readers>UINT32_MAX?UINT32_MAX:(uint32_t)p->domain.readers;
    ReleaseSRWLockExclusive(&p->domain.gate);return count;
}
static Kh02SavePipeline *router_active(Kh02SaveRouter *p) {
    Kh02SavePipeline *a;AcquireSRWLockExclusive(&p->domain.gate);a=p->domain.active;
    ReleaseSRWLockExclusive(&p->domain.gate);return a;
}
MEMBER_API Kh02SaveRouter *Kh02SaveRouterCreate(Kh02Lifecycle *life,const Kh02SavePipelineConfig *config) {
    Kh02SaveRouter *p;
    if(!life || !config || !config->stage || !config->refresh || !config->platform ||
       !config->read || !config->service || GetCurrentThreadId()!=life->thread ||
       config->base!=life->checkpoint_config.base)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->life=life;p->config=*config;p->config.owner=NULL;p->thread=GetCurrentThreadId();
    Kh02CallbackDomainInit(&p->domain);return p;
}
MEMBER_API void Kh02SaveRouterStop(Kh02SaveRouter *p) {
    Kh02CallbackPin pin={0};if(!p)return;InterlockedExchange(&p->closed,1);
    if(!Kh02CallbackStop(&p->domain,&pin))return;
    SELECTED(7,pin);
    if(pin.selected)Kh02SavePipelineStop(pin.selected);Kh02LifecycleStop(p->life);
    Kh02CallbackLeave(&pin);
}
/* Snapshot all roots at the same admission boundary. New callbacks can only
   reach these roots; preserve them even if concurrent Stop subsequently revokes
   them. Only the owner thread changes root identity or retained arrays.
   Destructors below call no originals and run outside the ingress lock. */
MEMBER_API uint32_t Kh02SaveRouterCollect(Kh02SaveRouter *p) {
    Kh02SavePipeline *active;Kh02ReceiptOwner *owner;Kh02CheckpointReceipts *checkpoint;
    uint32_t n;if(!p || GetCurrentThreadId()!=p->thread)return KH02_BIND_REFUSED;
    AcquireSRWLockExclusive(&p->domain.gate);
    if(p->domain.readers){ReleaseSRWLockExclusive(&p->domain.gate);return KH02_BIND_BUSY;}
    active=p->domain.active;owner=InterlockedCompareExchangePointer(&p->life->branch->active,NULL,NULL);
    checkpoint=p->life->checkpoint;ReleaseSRWLockExclusive(&p->domain.gate);
    n=0;for(uint32_t i=0;i<p->count;++i) {
        Kh02SavePipeline *candidate=p->held[i];
        if(candidate==active || Kh02SavePipelineDestroy(&candidate)!=KH02_BIND_OK)p->held[n++]=candidate;
    }p->count=n;
    n=0;for(uint32_t i=0;i<p->life->count;++i) {
        Kh02CheckpointReceipts *candidate=p->life->retained[i];
        if(candidate==checkpoint || Kh02CheckpointDestroy(&candidate)!=KH02_BANK_OK)p->life->retained[n++]=candidate;
    }p->life->count=n;
    n=0;for(uint32_t i=0;i<p->life->branch->count;++i) {
        Kh02ReceiptOwner *candidate=p->life->branch->retained[i];uint32_t keep=candidate==owner;
        for(uint32_t j=0;j<p->count;++j)if(p->held[j]->config.owner==candidate)keep=1;
        for(uint32_t j=0;j<p->life->count;++j)if(p->life->retained[j]->parent==candidate)keep=1;
        if(keep || Kh02ReceiptOwnerDestroy(&candidate)!=KH02_BANK_OK)p->life->branch->retained[n++]=candidate;
    }p->life->branch->count=n;return KH02_BIND_OK;
}
/* Retired names need not be retained: the namespace root is sealed on first
   attachment and each strictly increasing epoch can be attached only once. */
MEMBER_API uint32_t Kh02SaveRouterAttach(Kh02SaveRouter *p,const unsigned char *raw,uint32_t size,
    const char *prefix,uint32_t length) {
    Kh02SavePipeline *next;Kh02ReceiptScope scope;Kh02SavePipelineConfig config;
    char suffix[24];int suffix_length;uint32_t root_length;
    if(!p || GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->closed,0,0) || router_active(p))return KH02_BIND_REFUSED;
    if(Kh02SaveRouterCollect(p)!=KH02_BIND_OK)return KH02_BIND_BUSY;
    if(p->count==8){Kh02SaveRouterStop(p);return KH02_BIND_CAPACITY;}
    config=p->config;config.owner=Kh02LifecycleCurrent(p->life);if(!config.owner)return KH02_BIND_REFUSED;
    scope=p->life->branch->scope;scope.load_id=Kh02LifecycleEpoch(p->life);
    if(!scope.load_id || scope.load_id<=p->attached_epoch || !prefix || !length || length>64)return KH02_BIND_REFUSED;
    suffix_length=snprintf(suffix,sizeof(suffix),"-%llu",(unsigned long long)scope.load_id);
    if(suffix_length<=0 || (uint32_t)suffix_length>=length)return KH02_BIND_REFUSED;
    root_length=length-(uint32_t)suffix_length;
    if(root_length>40 || memcmp(prefix+root_length,suffix,(uint32_t)suffix_length))return KH02_BIND_REFUSED;
    if(p->attached_epoch && (strlen(p->namespace_root)!=root_length || memcmp(prefix,p->namespace_root,root_length)))return KH02_BIND_REFUSED;
    next=Kh02SavePipelineCreate(&config,&scope,raw,size,prefix,length);if(!next)return KH02_BIND_REFUSED;
    if(!p->attached_epoch)memcpy(p->namespace_root,prefix,root_length);
    p->attached_epoch=scope.load_id;p->held[p->count++]=next;
    if(InterlockedCompareExchange(&p->closed,0,0) || !Kh02CallbackPublish(&p->domain,next)) {
        Kh02SavePipelineStop(next);return KH02_BIND_REFUSED;
    }return KH02_BIND_OK;
}
MEMBER_API unsigned char Kh02SaveRouterStage(Kh02SaveRouter *p,uintptr_t manager,uint32_t index) {
    Kh02CallbackPin pin={0};unsigned char r;if(!p || !Kh02CallbackEnter(&p->domain,&pin))return 0;
    SELECTED(4,pin);r=pin.selected?Kh02SavePipelineStage(pin.selected,manager,index):p->config.stage(manager,index);
    Kh02CallbackLeave(&pin);return r;
}
MEMBER_API uintptr_t Kh02SaveRouterRefresh(Kh02SaveRouter *p,uintptr_t manager,uint32_t index) {
    Kh02CallbackPin pin={0};uintptr_t r;if(!p || !Kh02CallbackEnter(&p->domain,&pin))return 0;
    SELECTED(5,pin);r=pin.selected?Kh02SavePipelineRefresh(pin.selected,manager,index):p->config.refresh(manager,index);
    Kh02CallbackLeave(&pin);return r;
}
MEMBER_API unsigned char Kh02SaveRouterPlatform(Kh02SaveRouter *p,uintptr_t a,uintptr_t b,uintptr_t c,
    uintptr_t d,uintptr_t e,uintptr_t caller) {
    Kh02CallbackPin pin={0};unsigned char r=0;if(!p || !Kh02CallbackEnter(&p->domain,&pin))return 0;
    SELECTED(6,pin);if(pin.selected)r=Kh02SavePipelinePlatform(pin.selected,a,b,c,d,e,caller);
    else if(caller!=p->config.base+0x53ee44)r=p->config.platform(a,b,c,d,e);
    Kh02CallbackLeave(&pin);return r;
}
MEMBER_API uintptr_t Kh02SaveRouterCapture(Kh02SaveRouter *p,uintptr_t context) {
    Kh02CallbackPin pin={0};uintptr_t r;if(!p || !Kh02CallbackEnter(&p->domain,&pin))return 0;
    r=Kh02LifecycleCapture(p->life,context);Kh02CallbackLeave(&pin);return r;
}
MEMBER_API uintptr_t Kh02SaveRouterRestore(Kh02SaveRouter *p,uintptr_t context,uint32_t mode) {
    Kh02CallbackPin pin={0};uintptr_t r;uint32_t alone,drain;
    if(!p || !Kh02CallbackTransition(&p->domain,&pin,&alone))return 0;
    if(!alone){if(pin.selected)Kh02SavePipelineStop(pin.selected);Kh02LifecycleStop(p->life);}
    r=pin.selected && alone?Kh02SaveLifecycleRestore(pin.selected,p->life,context,mode,&drain):Kh02LifecycleRestore(p->life,context,mode);
    Kh02CallbackLeave(&pin);Kh02SaveRouterCollect(p);return r;
}
MEMBER_API unsigned char Kh02SaveRouterLoad(Kh02SaveRouter *p,uintptr_t manager,uint32_t index) {
    Kh02CallbackPin pin={0};unsigned char r;uint32_t alone,drain;
    if(!p || !Kh02CallbackTransition(&p->domain,&pin,&alone))return 0;
    if(!alone){if(pin.selected)Kh02SavePipelineStop(pin.selected);Kh02LifecycleStop(p->life);}
    r=pin.selected && alone?Kh02SaveLifecycleLoad(pin.selected,p->life,manager,index,&drain):Kh02LifecycleLoad(p->life,manager,index);
    Kh02CallbackLeave(&pin);Kh02SaveRouterCollect(p);return r;
}
MEMBER_API uint32_t Kh02SaveRouterDestroy(Kh02SaveRouter **handle) {
    Kh02SaveRouter *p;Kh02CallbackPin pin={0};uint32_t alone;
    if(!handle)return KH02_BIND_REFUSED;p=*handle;if(!p)return KH02_BIND_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BIND_REFUSED;
    /* External hook exclusion/join is still required. */
    Kh02SaveRouterStop(p);if(!Kh02CallbackTransition(&p->domain,&pin,&alone))return KH02_BIND_BUSY;
    Kh02CallbackLeave(&pin);if(!alone || Kh02SaveRouterCollect(p)!=KH02_BIND_OK || p->count)return KH02_BIND_BUSY;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BIND_BUSY;*handle=NULL;return KH02_BIND_OK;
}
