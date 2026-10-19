#include "kh02_difficulty_observation.h"
extern uint32_t Kh02ObjectiveBeforeTransitionV116(void);
extern void Kh02ObjectiveAutoV120(void);
/* Additive mixed dispatcher composition; live item command remains unavailable. */
#include "kh02_fresh_replay_session_entry_candidate.c"
#include "kh02_fresh_replay_entry_candidate.c"
#if defined(KH02_CLEAR_LAYOUT_OBSERVATION) || defined(KH02_STRUCTURED_CLEAR)
#include "kh02_clear_layout_observation.h"
#ifdef KH02_STRUCTURED_CLEAR
#include "kh02_structured_clear_candidate.h"
#endif
#else
#include "kh02_clear_content_candidate.h"
#endif
#include "kh02_save_hooks_candidate.c"
typedef struct Kh02SaveHarnessEvent {
    uint32_t kind,thread;uint64_t epoch,result;uint32_t known,complete;
} Kh02SaveHarnessEvent;
static Kh02Lifecycle *harness_life;static Kh02SaveRouter *harness_router;
#ifdef KH02_REUSABLE_SAVE_PIPELINE
#define KH02_HARNESS_CAP 128
#else
#define KH02_HARNESS_CAP 32
#endif
static Kh02SaveHarnessEvent harness_events[KH02_HARNESS_CAP];static volatile LONG harness_count,harness_overflow;
static DWORD harness_thread;
static Kh02DiskLookup *harness_history_lookup;
static Kh02ResumeSaveEntry *harness_resume;
static Kh02SaveHarnessEvent *harness_begin(uint32_t kind);
static void harness_end(Kh02SaveHarnessEvent *e,uint64_t result);
static int harness_read(void *unused,uint64_t address,void *out,uint32_t size);
static __attribute__((unused)) uint32_t harness_clear_content(void *unused,uintptr_t primary,uintptr_t checkpoint) {
#if defined(KH02_CLEAR_LAYOUT_OBSERVATION) || defined(KH02_STRUCTURED_CLEAR)
    /* Observe once inside the existing authenticated fresh-copy boundary.
       Failure to observe is evidence, never a reason to revoke the proven owner. */
    (void)unused;
    harness_end(harness_begin(15),Kh02DifficultyObserve(harness_read,NULL,harness_router->config.base,primary-0x338));
    harness_end(harness_begin(11),Kh02ClearLayoutObserve(harness_read,NULL,primary));
    harness_end(harness_begin(12),Kh02ClearLayoutObserve(harness_read,NULL,checkpoint));
#ifdef KH02_STRUCTURED_CLEAR
    uint32_t status=Kh02StructuredClearInitialize(primary,checkpoint);
    harness_end(harness_begin(10),status);
    harness_end(harness_begin(13),Kh02ClearLayoutObserve(harness_read,NULL,primary));
    harness_end(harness_begin(14),Kh02ClearLayoutObserve(harness_read,NULL,checkpoint));
    return status==1;
#else
    return 1;
#endif
#else
    Kh02SaveHarnessEvent *e;uint32_t result;(void)unused;e=harness_begin(10);
    /* Historical failed v15/v16 adapter retained only for comparison. */
    result=Kh02ClearContentInitialize(primary+0x1f0,checkpoint+0x1f0);harness_end(e,result);return result==KH02_CLEAR_OK;
#endif
}
#ifdef KH02_FRESH_SETUP_SELFTEST
static Kh02SessionCandidate test_mixed_session;
static Kh02MixedDelivery *test_mixed_delivery;
static Kh02GameGrantContext test_mixed_game;
static uint32_t test_mixed_allowed=1;
static uint32_t test_mixed_active(void *unused){(void)unused;return test_mixed_allowed;}
MEMBER_API uint32_t Kh02MixedTestCreate(uint64_t cache) {
    if(test_mixed_delivery || !harness_router ||
       Kh02SessionInit(&test_mixed_session,harness_life->branch->scope.cookie)!=KH02_GATE_OK)return 0;
    test_mixed_game=(Kh02GameGrantContext){harness_router->config.base,cache,&test_mixed_session,
        harness_router->config.read,harness_router->config.read_context,test_mixed_active,NULL};
    test_mixed_delivery=Kh02MixedDeliveryCreate(harness_router,&test_mixed_game);return test_mixed_delivery!=NULL;
}
MEMBER_API void Kh02MixedTestCache(uint64_t cache){test_mixed_game.cache=cache;}
MEMBER_API uint32_t Kh02MixedTestExecute(uint64_t epoch,uint64_t sequence,uint32_t kind,
    const unsigned char *proposal,uint32_t size,Kh02MixedOutcome *out) {
    return Kh02MixedDeliveryExecute(test_mixed_delivery,epoch,sequence,kind,proposal,size,out);
}
MEMBER_API void Kh02MixedTestRevoke(void){test_mixed_allowed=0;}
#endif
#ifdef KH02_FRESH_SAVE_HARNESS
static Kh02RestoreHook harness_copy_hook;
static Kh02FreshSaveEntry *harness_fresh;
static __declspec(noinline) uintptr_t harness_copy(uintptr_t destination,uintptr_t source) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    Kh02SaveHarnessEvent *event=(caller==harness_router->config.base+0x56e280 || caller==harness_router->config.base+0x56e277)?harness_begin(9):NULL;
    uintptr_t result=Kh02FreshSaveEntryCopy(harness_fresh,caller,destination,source);
    if(caller==harness_router->config.base+0x56e280 && Kh02LifecycleCurrent(harness_life))InterlockedExchange(&harness_resume->spent,1);
    /* Activation must not depend on diagnostic event capacity. */
    if(caller==harness_router->config.base+0x56e280)Kh02ObjectiveAutoV120();
    if(event)harness_end(event,Kh02LifecycleCurrent(harness_life)!=NULL);
    return result;
}
#endif
static int harness_read(void *unused,uint64_t address,void *out,uint32_t size) {
    (void)unused;return hook_read((uintptr_t)address,out,size);
}
static Kh02SaveHarnessEvent *harness_begin(uint32_t kind) {
    LONG at;Kh02SaveHarnessEvent *e;
    for(;;){at=InterlockedCompareExchange(&harness_count,0,0);
        if(at>=KH02_HARNESS_CAP){InterlockedExchange(&harness_overflow,1);return NULL;}
        if(InterlockedCompareExchange(&harness_count,at+1,at)==at)break;}
    e=&harness_events[at];e->kind=kind;e->thread=GetCurrentThreadId();return e;
}
static void harness_end(Kh02SaveHarnessEvent *e,uint64_t result) {
    if(!e)return;e->result=result;e->epoch=Kh02LifecycleEpoch(harness_life);
    e->known=Kh02LifecycleCurrent(harness_life)!=NULL;MemoryBarrier();InterlockedExchange((volatile LONG*)&e->complete,1);
}
static void harness_retention(void) {
    /* Owner-thread counters only; no pointer or new process read in evidence. */
    if(GetCurrentThreadId()!=harness_thread)return;
    harness_end(harness_begin(8),(uint64_t)harness_router->count |
        ((uint64_t)harness_life->count<<16) | ((uint64_t)harness_life->branch->count<<32));
}
static unsigned char harness_load(uintptr_t p,uint32_t i) {
    if(!Kh02ObjectiveBeforeTransitionV116())return 0;
    /* Kind7: low32=index, bit32=expected-manager match. No process address or
       unbounded memory access is recorded. Keep the existing wire layout. */
    Kh02SaveHarnessEvent *input=harness_begin(7);
    harness_end(input,(uint64_t)i|((uint64_t)(p==harness_life->checkpoint_config.base+0x4485500)<<32));
    Kh02SaveHarnessEvent *e=harness_begin(1);harness_end(harness_begin(16),Kh02DifficultyObserveRoot(harness_read,NULL,harness_router->config.base));
    unsigned char r=Kh02ResumeSaveEntryLoad(harness_resume,p,i);
    difficulty_after_load(r);
    if(r)Kh02ObjectiveAutoV120();
    if(r && Kh02LifecycleCurrent(harness_life))harness_end(harness_begin(17),Kh02DifficultyObserveRoot(harness_read,NULL,harness_router->config.base));harness_end(e,r);harness_retention();return r;
}
static uintptr_t harness_capture(uintptr_t p) {
    Kh02SaveHarnessEvent *e=harness_begin(2);uintptr_t r=Kh02SaveRouterCapture(harness_router,p);harness_end(e,r);return r;
}
static uintptr_t harness_restore(uintptr_t p,uint32_t m) {
    if(!Kh02ObjectiveBeforeTransitionV116())return 0;
    Kh02SaveHarnessEvent *e=harness_begin(3);uintptr_t r=Kh02SessionSaveEntryRestore(harness_resume,p,m);Kh02ObjectiveAutoV120();harness_end(e,r);harness_retention();return r;
}
static unsigned char harness_stage(uintptr_t p,uint32_t i) {
    Kh02SaveHarnessEvent *e=harness_begin(4);unsigned char r=Kh02SaveRouterStage(harness_router,p,i);harness_end(e,r);return r;
}
static uintptr_t harness_refresh(uintptr_t p,uint32_t i) {
    Kh02SaveHarnessEvent *e=harness_begin(5);uintptr_t r=Kh02SaveRouterRefresh(harness_router,p,i);harness_end(e,r);return r;
}
static unsigned char harness_platform(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t caller) {
    Kh02SaveHarnessEvent *event=harness_begin(6);unsigned char r=Kh02SaveRouterPlatform(harness_router,a,b,c,d,e,caller);
    harness_end(event,r);return r;
}
/* Scope/index/history/service are prevalidated owned inputs retained externally.
   This does not initialize synthetic membership or infer any save association. */
MEMBER_API uint32_t Kh02SaveHarnessCreate(unsigned char *base,Kh02DiskLookup *lookup,const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count,Kh02PublicationService *service) {
    Kh02SaveHookOriginals o;Kh02DiskLoadConfig disk;Kh02CheckpointConfig cp={0};Kh02SavePipelineConfig config;
    uint32_t status;if(harness_life || !lookup || !scope || !service)return KH02_BAD_REQUEST;
    harness_history_lookup=lookup;status=Kh02SaveHooksPrepare(base,&o);if(status!=KH02_OK)return status;harness_thread=GetCurrentThreadId();
    disk=(Kh02DiskLoadConfig){(uintptr_t)base,scope->cookie,scope->load_id,harness_read,NULL,o.load,lookup};
    cp.base=(uintptr_t)base;cp.capture=o.capture;cp.restore=o.restore;
    harness_life=Kh02LifecycleCreate(&disk,scope,history,count,ids,id_count,&cp);if(!harness_life)return KH02_BAD_REQUEST;
    config=(Kh02SavePipelineConfig){(uintptr_t)base,harness_read,NULL,o.stage,o.refresh,o.platform,NULL,service};
    harness_router=Kh02SaveRouterCreate(harness_life,&config);
    if(!harness_router){Kh02LifecycleStop(harness_life);return KH02_BAD_REQUEST;}return KH02_OK;
}
MEMBER_API uint32_t Kh02SaveHarnessPublish(void) {
    Kh02SaveHookCallbacks c={harness_load,harness_capture,harness_restore,harness_stage,harness_refresh,harness_platform};
    uint32_t status;if(!harness_router || !harness_resume || GetCurrentThreadId()!=harness_thread)return KH02_BAD_REQUEST;
#ifdef KH02_FRESH_SAVE_HARNESS
    if(!harness_fresh)return KH02_BAD_REQUEST;
#endif
    status=Kh02SaveHooksPublish(&c);
#ifdef KH02_FRESH_SAVE_HARNESS
    /* Install the save barrier first. A partial publish never falls through to
       unassociated save publication; all callback state remains retained. */
    if(status==KH02_OK)status=Kh02RestoreHookPublish(&harness_copy_hook);
#endif
    if(status!=KH02_OK)Kh02SaveRouterStop(harness_router);return status;
}
#ifdef KH02_FRESH_SAVE_HARNESS
MEMBER_API uint32_t Kh02SaveHarnessPrepareFresh(const unsigned char *empty,uint32_t empty_size,
    const unsigned char *raw,uint32_t raw_size,const char *prefix,uint32_t prefix_size,
    Kh02FreshPermit permit,void *permit_context) {
    Kh02FreshConfig config;uint32_t status;
    if(!harness_router || harness_fresh || !permit || GetCurrentThreadId()!=harness_thread)return KH02_BAD_REQUEST;
    status=Kh02RestoreHookPrepare(&harness_copy_hook,
        (unsigned char*)(uintptr_t)(harness_life->checkpoint_config.base+0x445730),2,harness_copy);
    if(status!=KH02_OK)goto failed;
    config=(Kh02FreshConfig){harness_life->checkpoint_config.base,harness_read,NULL,
        (Kh02FreshCopy)harness_copy_hook.trampoline,permit,permit_context,harness_clear_content,NULL};
    status=Kh02LifecycleArmFresh(harness_life,&config,empty,empty_size);
    if(status!=KH02_BANK_OK)goto failed;
    harness_fresh=Kh02FreshSaveEntryPrepare(harness_router,raw,raw_size,prefix,prefix_size);
    if(harness_fresh)return KH02_OK;
    status=KH02_BAD_REQUEST;
failed:
    Kh02SaveRouterStop(harness_router);return status;
}
#endif
MEMBER_API uint32_t Kh02SaveHarnessAttach(const unsigned char *raw,uint32_t size,const char *prefix,uint32_t length) {
    return harness_router?Kh02SaveRouterAttach(harness_router,raw,size,prefix,length):KH02_BIND_REFUSED;
}
MEMBER_API void Kh02SaveHarnessStop(void){if(harness_router)Kh02SaveRouterStop(harness_router);}
#ifdef KH02_SAVE_HOOKS_SELFTEST
/* Only after synchronous synthetic calls have joined. Never an alive-game reader. */
MEMBER_API uint32_t Kh02SaveHarnessTestEvents(Kh02SaveHarnessEvent *out,uint32_t capacity,uint32_t *overflow) {
    uint32_t count=(uint32_t)InterlockedCompareExchange(&harness_count,0,0);
    if(!out || !overflow || capacity<count)return 0;memcpy(out,harness_events,count*sizeof(*out));
    *overflow=(uint32_t)InterlockedCompareExchange(&harness_overflow,0,0);return count;
}
#endif

MEMBER_API uint32_t Kh02SaveHarnessPrepareResume(const unsigned char *raw,uint32_t size,const char *prefix,uint32_t length,uint32_t index,Kh02ResumePermit permit,void *context) {
 if(harness_resume)return 2;harness_resume=Kh02ResumeSaveEntryPrepare(harness_router,raw,size,prefix,length,index,permit,context);return harness_resume?1:2;
}

#ifdef KH02_FRESH_SETUP_SELFTEST
static Kh02ReceiptOwner *test_borrow_owner;
static Kh02ReceiptLease test_borrow_lease;
MEMBER_API uint32_t Kh02RetiringTestCounts(uint32_t *out) {
    if(!out || GetCurrentThreadId()!=harness_thread)return 0;
    out[0]=harness_router->count;out[1]=harness_life->count;out[2]=harness_life->branch->count;return 1;
}
MEMBER_API uint32_t Kh02RetiringTestCollect(void) {return Kh02SaveRouterCollect(harness_router);}
MEMBER_API uint32_t Kh02RetiringTestBorrow(void) {
    Kh02MembershipTicket ticket={0};if(test_borrow_owner)return 0;
    test_borrow_owner=Kh02LifecycleCurrent(harness_life);if(!test_borrow_owner)return 0;
    if(Kh02ReceiptOwnerStageBegin(test_borrow_owner,&ticket)!=KH02_MEMBER_OK ||
       Kh02ReceiptOwnerStageFinish(test_borrow_owner,&ticket,1,&test_borrow_lease)!=KH02_MEMBER_OK)return 0;
    return 1;
}
MEMBER_API uint32_t Kh02RetiringTestRelease(unsigned char *out,uint32_t capacity,uint32_t *size) {
    if(!test_borrow_owner || Kh02ReceiptOwnerRead(test_borrow_owner,&test_borrow_lease,out,capacity,size)!=KH02_BANK_OK ||
       Kh02ReceiptOwnerRelease(test_borrow_owner,&test_borrow_lease)!=KH02_BANK_OK)return 0;
    test_borrow_owner=NULL;return 1;
}
/* Caller has joined callbacks. Detach only publication, leaving the exact
   current owner intact to exercise rejected reattachment namespaces. */
MEMBER_API uint32_t Kh02RetiringTestReattach(const char *prefix,uint32_t length) {
    Kh02CallbackPin pin={0};uint32_t alone;
    if(GetCurrentThreadId()!=harness_thread || !Kh02CallbackTransition(&harness_router->domain,&pin,&alone))return 0;
    Kh02CallbackLeave(&pin);if(!alone)return 0;
    return Kh02SaveRouterAttach(harness_router,harness_resume->baseline,KH02_BIND_SIZE,prefix,length);
}
#endif

#ifdef KH02_FRESH_SETUP_SELFTEST
MEMBER_API uint32_t Kh02RetiringTestNextNamespace(void) {
    char wrong[65];int length;
    if(Kh02SaveRouterLoad(harness_router,harness_life->checkpoint_config.base+0x4485500,1)!=1 ||
       !Kh02LifecycleCurrent(harness_life))return 0;
    length=snprintf(wrong,sizeof(wrong),"different-%llu",(unsigned long long)Kh02LifecycleEpoch(harness_life));
    if(Kh02SaveRouterAttach(harness_router,harness_resume->baseline,KH02_BIND_SIZE,wrong,(uint32_t)length)==KH02_BIND_OK)return 0;
    return session_attach(harness_resume)==KH02_BIND_OK;
}
#endif

#ifdef KH02_FRESH_SETUP_SELFTEST
MEMBER_API uint32_t Kh02HistoryTestAppend(const Kh02ReceiptIdentity *history,uint32_t count,uint64_t cookie) {
    Kh02ReceiptScope scope=harness_life->branch->scope;scope.cookie=cookie;
    return Kh02HistoryRouterAppend(harness_router,harness_history_lookup,&scope,history,count);
}
MEMBER_API uint32_t Kh02HistoryTestExecute(const unsigned char *bytes,uint32_t size,Kh02ReceiptIdentityEffect effect,void *context) {
    return Kh02HistoryRouterExecute(harness_router,bytes,size,effect,context);
}
MEMBER_API uint32_t Kh02HistoryTestCurrent(unsigned char *out,uint32_t capacity,uint32_t *size) {
    Kh02ReceiptOwner *owner=Kh02LifecycleCurrent(harness_life);Kh02MembershipTicket ticket={0};Kh02ReceiptLease lease={0};uint32_t status;
    if(!owner || Kh02ReceiptOwnerStageBegin(owner,&ticket)!=KH02_MEMBER_OK ||
       Kh02ReceiptOwnerStageFinish(owner,&ticket,1,&lease)!=KH02_MEMBER_OK)return 0;
    status=Kh02ReceiptOwnerRead(owner,&lease,out,capacity,size);
    return Kh02ReceiptOwnerRelease(owner,&lease)==KH02_BANK_OK && status==KH02_BANK_OK;
}
#endif

#ifdef KH02_FRESH_SETUP_SELFTEST
MEMBER_API uint32_t Kh02HistoryTestDetach(void) {
    Kh02CallbackPin pin={0};uint32_t alone;
    if(GetCurrentThreadId()!=harness_thread || !Kh02CallbackTransition(&harness_router->domain,&pin,&alone))return 0;
    Kh02CallbackLeave(&pin);return alone;
}
#endif
