#include "resume_bounds_v254.h"
/* Additive history-command effects-off transport. Reuses the existing owner-thread message-hook
   protocol; no item commands or world-routing rules are accepted. */
#ifdef KH02_CONTROLLED_FRESH_REPLAY_SETUP
#include "kh02_fresh_replay_setup_candidate.c"
#else
#include "kh02_controlled_session_setup_candidate.c"
#endif
#if defined(KH02_FRESH_TRANSPORT_RUNTIME) && (defined(KH02_FRESH_SETUP_SELFTEST) || defined(KH02_SAVE_HOOKS_SELFTEST) || defined(KH02_RESTORE_HOOK_SELFTEST))
#error Runtime transport must not contain synthetic controls
#endif
#define FRESH_SETUP_CAP KH02_SETUP_CAP_V254
#ifndef KH02_CONTROLLED_BLOCK_EXTRA
#define KH02_CONTROLLED_BLOCK_EXTRA
#endif
#ifndef KH02_CONTROLLED_AFTER_SETUP
#define KH02_CONTROLLED_AFTER_SETUP(value) (value)
#endif
#ifndef KH02_CONTROLLED_BEFORE_STOP
#define KH02_CONTROLLED_BEFORE_STOP() 1
#endif
#ifndef KH02_CONTROLLED_EXTRA_COMMAND
#define KH02_CONTROLLED_EXTRA_COMMAND(block,command,status,handled) ((void)(block),(void)(command),(void)(status),(void)(handled))
#endif
#ifndef KH02_CONTROLLED_SNAPSHOT
#define KH02_CONTROLLED_SNAPSHOT(block) ((void)(block))
#endif
typedef struct Kh02FreshTransportBlock {
    Kh02RouteBlock transport;uint32_t setup_size,reserved;
    unsigned char setup[FRESH_SETUP_CAP];
    uint32_t evidence_complete,evidence_sequence,count,overflow;
    Kh02SaveHarnessEvent events[KH02_HARNESS_CAP];
    volatile LONG64 authority;
    uint64_t requested_authority,acknowledged_authority,epoch,attempt_sequence;
    uint32_t kind,proposal_size;
    unsigned char proposal[KH02_RECEIPT_MAX_BYTES];
    Kh02MixedOutcome outcome;
    uint64_t query_epoch;
    uint32_t query_size,query_reserved;
    unsigned char receipts[KH02_RECEIPT_MAX_BYTES];
    KH02_CONTROLLED_BLOCK_EXTRA
} Kh02FreshTransportBlock;
static Kh02FreshTransportBlock *fresh_map;
static HANDLE fresh_mapping;
static DWORD fresh_thread;
static uint64_t fresh_cookie;
static uint32_t fresh_served,fresh_spent;
static volatile LONG fresh_servicing;
static uint64_t accepted_authority,active_authority;
static uint32_t controlled_transport_permit(void *unused) {
    (void)unused;
    return fresh_map && active_authority && active_authority==accepted_authority &&
        (uint64_t)InterlockedCompareExchange64(&fresh_map->authority,0,0)==active_authority && storage_permit(NULL);
}
MEMBER_API uint32_t Kh02FreshTransportLiveEnabled(void){
#ifdef KH02_FRESH_TRANSPORT_RUNTIME
    return 1;
#else
    return 0;
#endif
}
MEMBER_API uint32_t Kh02FreshTransportSize(void){return sizeof(Kh02FreshTransportBlock);}
MEMBER_API uint32_t Kh02FreshTransportHeaderBindingEnabled(void){
#ifdef KH02_SAVE_HEADER_BINDING
    return 1;
#else
    return 0;
#endif
}
MEMBER_API uint32_t Kh02ValidateFreshTransport(const Kh02RoutePolicy *p) {
    const Kh02RouteRule zero[8]={0};
    return !p || !p->owner_cookie || p->generation!=1 || p->count || p->reserved ||
        memcmp(p->rules,zero,sizeof(zero))?2:0;
}
#ifdef KH02_FRESH_SETUP_SELFTEST
static unsigned char *fresh_test_image;static uint64_t fresh_test_created;
static Kh02StorageCheck fresh_test_check;
#endif
/* Wire: fixed 48-byte header, aligned identities, then exact UTF-8 seed.
   The shared input is copied before parsing or entering any coordinator lock. */
typedef struct Kh02HistoryCommandWire {
    uint32_t magic,version,size,count;
    uint64_t cookie,load_id;
    uint32_t team,slot,seed_size,reserved;
} Kh02HistoryCommandWire;
_Static_assert(sizeof(Kh02HistoryCommandWire)==48,"history command ABI");
static uint32_t history_command(Kh02FreshTransportBlock *b) {
    uint32_t size=b->setup_size,status=KH02_BANK_REFUSED;
    uint64_t authority=b->requested_authority;accepted_authority=0;b->acknowledged_authority=0;
    unsigned char *copy=NULL;Kh02HistoryCommandWire h;Kh02ReceiptScope scope;
    if(!harness_router)return status;
    /* Disconnect before service is a pre-commit deferral, not a save fault. */
    if(!authority || (uint64_t)InterlockedCompareExchange64(&b->authority,0,0)!=authority)return KH02_BANK_BUSY;
    if(b->reserved || size<48 || size>48+4096*24+4096)goto done;
    copy=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!copy)goto done;
    memcpy(copy,b->setup,size);memcpy(&h,copy,48);
    if(h.magic!=0x4b484843 || h.version!=1 || h.size!=size || h.count>4096 ||
       !h.seed_size || h.seed_size>4096 || h.reserved ||
       (uint64_t)48+h.count*24+h.seed_size!=size)goto done;
    if(!storage_permit(NULL))goto done;
    scope=(Kh02ReceiptScope){h.cookie,h.load_id,h.team,h.slot,
        copy+48+h.count*24,h.seed_size,0};
    status=Kh02HistoryRouterAppend(harness_router,harness_history_lookup,&scope,
        (Kh02ReceiptIdentity*)(copy+48),h.count);
done:
    if(status==KH02_BANK_OK && (uint64_t)InterlockedCompareExchange64(&b->authority,0,0)==authority)
        b->acknowledged_authority=accepted_authority=authority;
    if(copy)VirtualFree(copy,0,MEM_RELEASE);
    if(status!=KH02_BANK_OK && status!=KH02_BANK_BUSY)Kh02SaveHarnessStop();
    return status;
}
static void fresh_service(Kh02FreshTransportBlock *b) {
    Kh02RoutePolicy policy;uint32_t seq,command,status=2,size,active,flight;unsigned char *copy;
    if(!b || b->transport.magic!=KH02_ROUTE_MAGIC || b->transport.version!=1 ||
       b->transport.size!=sizeof(*b))return;
    seq=b->transport.request_sequence;
    if(!seq || seq<=fresh_served || InterlockedCompareExchange(&fresh_servicing,1,0))return;
    policy=b->transport.policy;command=b->transport.command;
    if(Kh02ValidateFreshTransport(&policy) ||
       (fresh_cookie && (b!=fresh_map || fresh_cookie!=policy.owner_cookie || fresh_thread!=GetCurrentThreadId())))goto response;
    if(!fresh_cookie){fresh_map=b;fresh_cookie=policy.owner_cookie;fresh_thread=GetCurrentThreadId();}
    fresh_served=seq;
    if(command==2) {
        if(!KH02_CONTROLLED_BEFORE_STOP())status=5;
        else {
            fresh_spent=1;accepted_authority=0;Kh02GrantOwnerStop();Kh02SaveHarnessStop();
            status=harness_router && router_flight(harness_router)?5:8;
        }
    } else if(command==1 && !fresh_spent) {
        fresh_spent=1;size=b->setup_size;
        if(b->reserved || size<128 || size>FRESH_SETUP_CAP)goto response;
        copy=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copy)goto response;
        memcpy(copy,b->setup,size);
        uint64_t cookie;memcpy(&cookie,copy+16,8);
        if(cookie==fresh_cookie) {
#ifdef KH02_FRESH_SETUP_SELFTEST
            status=fresh_setup_arm(fresh_test_image,copy,size,fresh_test_created,fresh_test_check);
#else
            status=Kh02FreshSetupArm(copy,size);
#endif
        }
        VirtualFree(copy,0,MEM_RELEASE);
        status=KH02_CONTROLLED_AFTER_SETUP(status);
    } else if(command==4 && fresh_spent) {
        status=history_command(b);
    } else if((command==3 || command==5) && fresh_spent) {
        uint64_t authority=b->requested_authority;
        if(!authority || authority!=accepted_authority)goto response;
        active_authority=authority;
        if(controlled_transport_permit(NULL)) {
            if(command==3) {
                b->query_size=0;b->query_epoch=0;
                status=Kh02ControlledOwnerProbe(&b->query_epoch,b->receipts,sizeof(b->receipts),&b->query_size);
            } else {
                uint64_t epoch=b->epoch,attempt=b->attempt_sequence;
                uint32_t kind=b->kind;size=b->proposal_size;
                if(size>=56 && size<=KH02_RECEIPT_MAX_BYTES) {
                    copy=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                    if(copy) {
                        memcpy(copy,b->proposal,size);
                        status=Kh02ControlledOwnerExecute(epoch,attempt,kind,copy,size,&b->outcome);
                        VirtualFree(copy,0,MEM_RELEASE);
                    }
                }
            }
        }
        active_authority=0;
    } else {
        uint32_t handled=0;KH02_CONTROLLED_EXTRA_COMMAND(b,command,status,handled);
        if(!handled)status=2;
    }
response:
    KH02_CONTROLLED_SNAPSHOT(b);
    active=harness_router && !InterlockedCompareExchange(&harness_router->closed,0,0);
    flight=(harness_router?(uint32_t)router_flight(harness_router):0)+Kh02GrantOwnerFlight();
    if(command==2 && status==8 && !flight) {
        b->evidence_complete=0;
        b->count=(uint32_t)InterlockedCompareExchange(&harness_count,0,0);
        b->overflow=(uint32_t)InterlockedCompareExchange(&harness_overflow,0,0);
        memcpy(b->events,harness_events,sizeof(harness_events));b->evidence_sequence=seq;
        MemoryBarrier();InterlockedExchange((volatile LONG*)&b->evidence_complete,1);
    }
    b->transport.cleanup=(Kh02CleanupReceipt){KH02_ROUTE_MAGIC,1,sizeof(Kh02CleanupReceipt),seq,
        fresh_cookie?fresh_cookie:policy.owner_cookie,1,status,active,0,flight};
    b->transport.status=status;b->transport.active=active;
    MemoryBarrier();b->transport.response_sequence=seq;InterlockedExchange(&fresh_servicing,0);
}
MEMBER_API LRESULT CALLBACK Kh02FreshTransportHook(int code,WPARAM w,LPARAM l) {
    if(code==HC_ACTION && w==PM_REMOVE && l && ((MSG*)l)->message==0x8271) {
        if(!fresh_map) {
            wchar_t name[96];_snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APFreshTransport-%lu",GetCurrentProcessId());
            fresh_mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);
            if(fresh_mapping)fresh_map=MapViewOfFile(fresh_mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(*fresh_map));
        }
        fresh_service(fresh_map);
    }
    return CallNextHookEx(NULL,code,w,l);
}
#ifdef KH02_FRESH_SETUP_SELFTEST
MEMBER_API uint32_t Kh02FreshTransportTestBind(unsigned char *image,uint64_t created,Kh02StorageCheck check) {
    if(fresh_test_image || fresh_cookie || !image || !created || !check)return 2;
    fresh_test_image=image;fresh_test_created=created;fresh_test_check=check;return 1;
}
MEMBER_API void Kh02FreshTransportTestService(Kh02FreshTransportBlock *b,unsigned char *image,uint64_t created,Kh02StorageCheck check) {
    if(!fresh_cookie){fresh_test_image=image;fresh_test_created=created;fresh_test_check=check;}
    fresh_service(b);
}
MEMBER_API uint32_t Kh02FreshTransportTestSize(void){return sizeof(Kh02FreshTransportBlock);}
#endif
