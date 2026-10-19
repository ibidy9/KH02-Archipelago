/* Current shared-owner adapter for the live-proven objective reward substitution.
   Explicitly armed development candidate; no second writer or completion hook. */
#include "kh02_objective_read_v116.h"
extern uint32_t Kh02ObjectiveAuthorityV116(void);
typedef struct ObjectiveRequestV116 {
    uint64_t magic,epoch,cookie,load;uint32_t action,reserved;
} ObjectiveRequestV116;
typedef struct ObjectiveReplyV116 {
    uint64_t cookie,epoch,load,owner;
    uint32_t suppressed,reserved,states[51],padding;
} ObjectiveReplyV116;
_Static_assert(sizeof(ObjectiveRequestV116)==40,"objective request ABI");
_Static_assert(sizeof(ObjectiveReplyV116)==248,"objective reply ABI");
static uint32_t objective_owned_v116;
static DWORD objective_protection_v116;
static uint64_t objective_epoch_v116,objective_load_v116,objective_cookie_v116;
static Kh02ReceiptOwner *objective_receipt_v116;
static const unsigned char objective_original_v116[8]={0x5d,0x05,0,0x44,0x8b,0xf8,0x48,0x85};
static const unsigned char objective_patch_v116[8]={0x5d,0x05,0,0x45,0x8b,0xfc,0x48,0x85};

/* Same aligned exchange and instruction contract as the proven combined owner.
   Only the game thread at its command/frame/transition boundary may call this.
   Atomicity alone is NOT the quiescence argument. No completion call may be
   executing while the owner handles an ordinary command or top-level frame.
   Transition removal retains the old owner's nested-transition exception. */
/* Chest reward branch only: opened flag and finalizer remain vanilla. */
static uint32_t chest_owned_v116;
static DWORD chest_protection_v116;
static const unsigned char chest_original_v116[8]={0x8b,0xf8,0x85,0xc0,0x74,0x5b,0xe8,0x45};
static const unsigned char chest_patch_v116[8]={0x8b,0xf8,0x85,0xc0,0xeb,0x5b,0xe8,0x45};
static uint32_t chest_exchange_v116(uint32_t enable,uint32_t transition) {
    unsigned char *p=(unsigned char*)(uintptr_t)(game.base+0x484000);
    const unsigned char *from=enable?chest_original_v116:chest_patch_v116;
    const unsigned char *to=enable?chest_patch_v116:chest_original_v116;
    LONG64 expected,desired,found;DWORD old,ignored;int flushed,protected,original;
    if(GetCurrentThreadId()!=owner_thread || (flight>1 && (enable || !transition)))return 0;
    if(!enable && !chest_owned_v116)return 1;
    if(enable && chest_owned_v116)return !memcmp(p,chest_patch_v116,8);
    original=!enable && !memcmp(p,chest_original_v116,8);
    if(memcmp(p,from,8) && !original)return 0;
    if(!VirtualProtect(p,8,PAGE_EXECUTE_READWRITE,&old))return 0;
    if(enable){chest_protection_v116=old;chest_owned_v116=1;}
    memcpy(&expected,from,8);memcpy(&desired,to,8);
    found=original?expected:InterlockedCompareExchange64((volatile LONG64*)p,desired,expected);
    flushed=FlushInstructionCache(GetCurrentProcess(),p,8)!=0;
    protected=VirtualProtect(p,8,chest_protection_v116,&ignored)!=0;
    if(!flushed || !protected || found!=expected || memcmp(p,to,8))return 0;
    if(!enable)chest_owned_v116=0;
    return 1;
}
static uint32_t objective_exchange_core_v125(uint32_t enable,uint32_t transition) {
    unsigned char *p=(unsigned char*)(uintptr_t)(game.base+0x42e4b0);
    const unsigned char *from=enable?objective_original_v116:objective_patch_v116;
    const unsigned char *to=enable?objective_patch_v116:objective_original_v116;
    LONG64 expected,desired,found;DWORD old,ignored;int flushed,protected,original;
    if(GetCurrentThreadId()!=owner_thread || (flight>1 && (enable || !transition)))return 0;
    if(!enable && !objective_owned_v116)return 1;
    if(enable && objective_owned_v116)return !memcmp(p,objective_patch_v116,8);
    original=!enable && !memcmp(p,objective_original_v116,8);
    if(memcmp(p,from,8) && !original)return 0;
    if(!VirtualProtect(p,8,PAGE_EXECUTE_READWRITE,&old))return 0;
    if(enable){objective_protection_v116=old;objective_owned_v116=1;}
    memcpy(&expected,from,8);memcpy(&desired,to,8);
    found=original?expected:InterlockedCompareExchange64((volatile LONG64*)p,desired,expected);
    flushed=FlushInstructionCache(GetCurrentProcess(),p,8)!=0;
    protected=VirtualProtect(p,8,objective_protection_v116,&ignored)!=0;
    if(!flushed || !protected || found!=expected || memcmp(p,to,8))return 0;
    if(!enable)objective_owned_v116=0;
    return 1;
}
static uint32_t objective_exchange_v116(uint32_t enable,uint32_t transition) {
    uint32_t a,b;
    if(enable) {
        if(!objective_exchange_core_v125(1,transition))return 0;
        if(chest_exchange_v116(1,transition))return 1;
        /* Preserve uncertainty and attempt rollback; shutdown retries owned bytes. */
        (void)objective_exchange_core_v125(0,transition);return 0;
    }
    a=chest_exchange_v116(0,transition);
    b=objective_exchange_core_v125(0,transition);
    return a && b;
}
uint32_t Kh02ObjectiveBeforeTransitionV116(void) {
    if(!objective_owned_v116 && !chest_owned_v116)return 1;
    if(!objective_exchange_v116(0,1)) {
        InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);return 0;
    }
    objective_epoch_v116=objective_load_v116=objective_cookie_v116=0;
    objective_receipt_v116=NULL;return 1;
}
static int objective_binding_v116(void) {
    return active_flag && !fault && Kh02ControlledStartAuthorized() &&
        objective_receipt_v116 && objective_receipt_v116==Kh02LifecycleCurrent(life) &&
        objective_epoch_v116==Kh02LifecycleEpoch(life) &&
        objective_cookie_v116==session.gate.current.cookie &&
        Kh02ReceiptOwnerCheck(objective_receipt_v116)==KH02_MEMBER_OK;
}
/* AP save ownership survives network history revocation. This only owns
   the proven code substitution; reads/grants still require network authority.
   Called after admitted load/fresh/restore returns and on the existing frame
   boundary. No completion frame, alternate writer or new hook is introduced. */
void Kh02ObjectiveAutoV120(void) {
    Kh02ReceiptOwner *owner;
    uint64_t epoch;
    if(GetCurrentThreadId()!=owner_thread || flight>1 || busy)return;
    if(objective_owned_v116 && !objective_binding_v116()) {
        if(!Kh02ObjectiveBeforeTransitionV116())return;
    }
    if(objective_owned_v116 || !active_flag || fault ||
       !Kh02ControlledStartAuthorized())return;
    owner=Kh02LifecycleCurrent(life);epoch=Kh02LifecycleEpoch(life);
    if(!owner || !epoch || !session.gate.current.cookie ||
       Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK)return;
    if(!objective_exchange_v116(1,0)) {
        InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);return;
    }
    objective_epoch_v116=epoch;objective_load_v116=epoch;
    objective_cookie_v116=session.gate.current.cookie;objective_receipt_v116=owner;
}
static void Kh02ObjectiveServiceV116(void) { Kh02ObjectiveAutoV120(); }
uint32_t Kh02ObjectiveRequestV116(void *raw,uint32_t size,uint32_t capacity,uint32_t *written) {
    ObjectiveRequestV116 request;ObjectiveReplyV116 reply={0};
    Kh02SessionSample sample;Kh02ObjectiveStatesV116 observed;
    uint32_t status=2;uint64_t epoch;
    if(written)*written=0;
    if(!raw || !written || size!=sizeof(request) || capacity<sizeof(reply) ||
       GetCurrentThreadId()!=owner_thread || busy || flight)return 2;
    memcpy(&request,raw,sizeof(request));
    if(request.magic!=0x3631314f5041ULL || request.reserved ||
       request.action<2 || request.action>11)return 2;
    InterlockedIncrement(&flight);
    if(request.action==3) {
        status=Kh02ObjectiveBeforeTransitionV116()?1:5;goto done;
    }
    epoch=Kh02LifecycleEpoch(life);
    if(!Kh02ObjectiveAuthorityV116() || request.epoch!=epoch ||
       request.cookie!=session.gate.current.cookie || request.cookie!=goal_cookie ||
       request.load!=goal_load || epoch!=goal_epoch ||
       goal_receipt_owner!=Kh02LifecycleCurrent(life) ||
       !popup_sample_v111(&sample,epoch) ||
       Kh02ObjectiveReadV116(read_local,NULL,game.base,sample.world,sample.context,&observed)!=KH02_GATE_OK ||
       !popup_same_v111(&sample,epoch))goto done;
    if(!objective_owned_v116 || !objective_binding_v116() ||
       memcmp((void*)(uintptr_t)(game.base+0x42e4b0),objective_patch_v116,8))goto done;
    if(request.action==9) {status=menu_castle_stage150();goto done;}
    if(request.action==5 || request.action==6 || request.action==7 || request.action==8 || request.action==10 || request.action==11) {
        /* Diagnostic only. Proven structured record and switch spans; no
           interpretation of header fields, no new hooks, no game writes. */
        struct ProgressReplyV127 {
            uint64_t cookie,epoch,load,owner;
            uint32_t suppressed,reserved;
            unsigned char header[64],switches[8];
            uint32_t exits,reason;uint64_t hits;
            uint32_t darksides,darkside_reason;uint64_t darkside_hits;
            uint32_t rush,rush_reason;uint64_t rush_hits;
            uint32_t live_level,live_reserved;
        } progress={0};
        unsigned char header[64],switches[8];
        uint32_t live_level_again=0;
        _Static_assert(sizeof(progress)==168,"progress reply ABI");
        if(!read_local(NULL,sample.context+0x538,progress.header,64) ||
           !read_local(NULL,sample.context+0x3538,progress.switches,8) ||
           !read_local(NULL,sample.context+0x538,header,64) ||
           !read_local(NULL,sample.context+0x3538,switches,8) ||
           memcmp(progress.header,header,64) || memcmp(progress.switches,switches,8) ||
           !popup_same_v111(&sample,epoch))goto done;
        progress.cookie=request.cookie;progress.epoch=epoch;progress.load=request.load;
        progress.owner=sample.context;progress.suppressed=1;
        if(request.action>=6) {
            if(capacity<(request.action==11?168:request.action==10?160:request.action>=7?144:128))goto done;
            progress.exits=exit_cookie_v132==request.cookie?(exit_mask_v132 & (request.action>=8?15u:7u)):0;
            progress.reason=exit_reason_v132;progress.hits=exit_hits_v132;
        }
        if(request.action>=7) {
            progress.darksides=darkside_cookie_v144==request.cookie?darkside_mask_v144:0;
            progress.darkside_reason=darkside_reason_v144;progress.darkside_hits=darkside_hits_v144;
        }
        if(request.action>=10) {
            progress.rush=rush162.cookie==request.cookie?rush162.mask:0;
            if(progress.switches[6]&1)progress.rush|=16;
            progress.rush_reason=rush162.reason;progress.rush_hits=rush162.hits;
        }
        if(request.action==11) {
            /* Same validated attribute provider as the native level getter
               at 0x31c520; never change the serialized header or game state. */
            if(!read_local(NULL,sample.attributes+0x40,&progress.live_level,4) ||
               !read_local(NULL,sample.attributes+0x40,&live_level_again,4) ||
               progress.live_level!=live_level_again || progress.live_level<50 ||
               progress.live_level>99 || !popup_same_v111(&sample,epoch))goto done;
        }
        *written=request.action==11?168:request.action==10?160:request.action>=7?144:request.action==6?128:112;
        memcpy(raw,&progress,*written);status=1;goto done;
    }
    if(request.action==4) {
        struct ChestReplyV125 {uint64_t cookie,epoch,load,owner;uint32_t suppressed,reserved;unsigned char flags[16];} chest={0};
        unsigned char again[16];
        _Static_assert(sizeof(chest)==56,"chest reply ABI");
        if(!chest_owned_v116 || memcmp((void*)(uintptr_t)(game.base+0x484000),chest_patch_v116,8) ||
           !read_local(NULL,sample.context+0x3838,chest.flags,16) ||
           !read_local(NULL,sample.context+0x3838,again,16) ||
           memcmp(chest.flags,again,16) || !popup_same_v111(&sample,epoch))goto done;
        chest.cookie=request.cookie;chest.epoch=epoch;chest.load=request.load;
        chest.owner=sample.context;chest.suppressed=1;
        memcpy(raw,&chest,sizeof(chest));*written=sizeof(chest);status=1;goto done;
    }
    reply.cookie=request.cookie;reply.epoch=epoch;reply.load=request.load;
    reply.owner=sample.context;reply.suppressed=1;
    memcpy(reply.states,observed.states,sizeof(reply.states));
    memcpy(raw,&reply,sizeof(reply));*written=sizeof(reply);status=1;
done:
    InterlockedDecrement(&flight);return status;
}

