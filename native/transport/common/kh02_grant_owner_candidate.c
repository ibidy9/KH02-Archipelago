#include "kh02_grant_owner_candidate.h"
#include <string.h>
#include "kh02_opening_trace_v54.h"
#if defined(KH02_GRANT_OWNER_LIVE) && (defined(KH02_GRANT_OWNER_SELFTEST) || defined(KH02_SAVE_HOOKS_SELFTEST) || defined(KH02_RECEIPT_BANK_SELFTEST))
#error Live owner must not expose synthetic controls
#endif
void *kh02_style_request_trampoline,*kh02_stat_trampoline,*kh02_travel_trampoline,*kh02_default_ability_trampoline,*kh02_native_ability_trampoline,*kh02_guard_trampoline,*kh02_dodge_trampoline,*kh02_magic_predicate_trampoline,*kh02_menu_trampoline;
extern void kh02_style_request_thunk(void),kh02_stat_thunk(void),kh02_travel_thunk(void),kh02_default_ability_thunk(void),kh02_native_ability_thunk(void),kh02_guard_thunk(void),kh02_dodge_thunk(void),kh02_magic_predicate_thunk(void),kh02_menu_thunk(void);
typedef struct Entry {
    unsigned char *target,original[19],patch[19];size_t size;DWORD protection;uint32_t owned;
} Entry;
static Entry entries[25];
static void *kh02_ww_remote_trampoline;
static const unsigned char ww_remote_prefix[19]={0x48,0x89,0x5c,0x24,0x20,0x55,0x48,0x83,0xec,0x20,0x48,0x8b,0xda,0x41,0xb8,1,0,0,0};
static const unsigned char ww_close_prefix[16]={0x48,0x83,0xec,0x28,0xe8,0x07,0x1d,0x0f,0,0x48,0x85,0xc0,0x74,0x0c,0x48,0x8b};
static Kh02SessionCandidate session;
static Kh02Lifecycle *life;
static Kh02GameGrantContext game;
static DWORD owner_thread;
static uint64_t seen_epoch,sealed_nonce;
static volatile LONG active_flag,fault,flight,busy,attempted;
static uint32_t xp_halves185;
static Kh02DefaultAbilitySnapshot default_abilities;
static void diagnostic_epoch(void);
static void Kh02GoalObservationGap(void);
static void action_epoch_reset(void);
static void Kh02ObjectiveServiceV116(void);
uint32_t Kh02ObjectiveBeforeTransitionV116(void);
#ifdef KH02_GRANT_OWNER_SELFTEST
static uint32_t test_fail_second;
KH02_RECEIPT_API void Kh02GrantOwnerTestFailSecond(uint32_t value){test_fail_second=value;}
#endif
static const unsigned char stat_prefix[16]={0x4c,0x8b,0xdc,0x53,0x48,0x81,0xec,0xd0,0,0,0,0x45,0x0f,0x29,0x53,0x88};
static const unsigned char travel_prefix[18]={0x4c,0x89,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x55,0x53,0x57,0x48,0x8d,0x6c,0x24,0xc0};
static const unsigned char query_prefix[15]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x01,0x8b,0xda};
static const unsigned char recovery_prefix[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30};
static const unsigned char action_prefix[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20};
static const unsigned char magic_prefix[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18};
static const unsigned char menu_prefix[17]={0x48,0x89,0x5c,0x24,0x8,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xb9,0xb0,0x3,0x0,0x0};
static int read_local(void *unused,uint64_t address,void *out,uint32_t size) {
    SIZE_T got=0;(void)unused;
    return ReadProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)address,out,size,&got)&&got==size;
}
static uint32_t on_thread(void) {
    if(GetCurrentThreadId()==owner_thread)return 1;
    InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);return 0;
}
static uint32_t sync_epoch(void) {
    uint64_t epoch=Kh02LifecycleEpoch(life);
    if(epoch==seen_epoch)return 1;
    if(!Kh02ObjectiveBeforeTransitionV116())return 0;
    seen_epoch=epoch;game.cache=0;session.context=session.world=0;
    diagnostic_epoch();action_epoch_reset();Kh02GoalObservationGap();
    Kh02GateInvalidate(&session.gate);return 0;
}
static uint32_t is_active(void *unused) {
    (void)unused;
    return on_thread() && InterlockedCompareExchange(&active_flag,0,0) &&
        !InterlockedCompareExchange(&fault,0,0) && sync_epoch() && Kh02LifecycleCurrent(life)!=NULL;
}
#include "kh02_action_observer_v32.inc"
#include "kh02_magic_observation_v38.inc"
#include "kh02_magic_predicate_v41.inc"
#include "kh02_situation_observer_v46.inc"
#include "kh02_style_gate_v47.inc"
#include "kh02_meter_access_v48.inc"
#include "movement_owner_v209.inc"
static void depths_complete_v145(uint64_t,uint64_t,uint64_t);
static uint32_t tide_choice_begin(uint64_t*,uint64_t);
static int tide_choice_customize(void*,void*,void**,void**,uintptr_t);
static void tide_choice_configured(void*,unsigned char);
static void tide_choice_event(void*,uint32_t,uint32_t);
static void tide_choice_closed(void*,uint32_t,uint32_t);
static void TideChoiceServiceV87(void);
static void menu_ww_event150(void*,uint64_t);
static void rush_event162(void*,uint64_t);
#include "kh02_start_adapter_v55.inc"
#include "rush_observer_v162.inc"
#include "mickey_trace_v167.inc"
#include "menu_world_v150.inc"
static void menu_begin213(uint64_t,uint64_t,uint32_t,uint32_t);
static void menu_end213(uint64_t,uint32_t);
#include "menu_visibility_v151.inc"
#include "menu_order_v213.inc"
#include "menu_forest_v149.inc"
#include "menu_depths_v146.inc"
#include "kh02_combat_boost_v65.inc"
#include "kh02_goal_probe_v61.inc"
#include "kh02_zodiac_refresh_v99.inc"
#include "kh02_area_map_lookup_v101.inc"
#include "kh02_stat_owner_v105.inc"
#include "popup_ready_v273.h"
#include "kh02_stat_popup_owner_v111.inc"
#include "main_road_meter_v275.inc"
#include "kh02_custom_popup_owner_v115.inc"
#include "trap_probe_v242.inc"
#include "death_observation_v250.inc"
#include "deathlink_v249.inc"
#include "kh02_stat_trace_v108.inc"
#ifdef KH02_GRANT_OWNER_LIVE
#include "boss_broadcast_v192.inc"
#elif defined(KH02_GRANT_OWNER_SELFTEST)
#include "boss_broadcast_row_v194.h"
static _Thread_local BroadcastRow192 *broadcast_scope192;
#endif
#include "kh02_wipeout_trace_v143.inc"
#if defined(KH02_GRANT_OWNER_LIVE) || defined(KH02_GRANT_OWNER_SELFTEST)
#include "local_drop_owner_v193.inc"
#endif
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02NamedAwardNestedTest199(uint64_t context,uint64_t name,uint32_t depth) {
 if(depth>3)return;
 for(uint32_t i=0;i<depth;i++)InterlockedIncrement(&flight);
 stat_award_observe_v108((void*)(uintptr_t)context,name,-1);
 for(uint32_t i=0;i<depth;i++)InterlockedDecrement(&flight);
}
/* Synthetic encounter admission only; no new control exists in live DLL. */
KH02_RECEIPT_API uint32_t Kh02LocalDropTestQueue194(uint64_t serial,uint32_t group) {
    BroadcastRow192 r={0};uint64_t before=local_policy193.accepted;
    (void)local_child193;
    if(!local_trace193 && !local_init193())return 0;
    r.cookie=session.gate.current.cookie;r.epoch=Kh02LifecycleEpoch(life);r.serial=serial;r.reserved=group;
    local_parent193(&r);return local_policy193.accepted==before+1;
}
#endif
#include "kh02_phantom_status_v68.inc"
#include "goal_publication_v72.inc"
#include "world_exit_v132.inc"
#include "kh02_objective_owner_v116.inc"
#include "zodiac_observer_v83.inc"
#include "confirm_observer_v86.inc"
#include "tide_choice_v87.inc"
void kh02_stat_capture(void *cache) {
    Kh02SessionSample sample;
    InterlockedIncrement(&flight);
    Kh02ObjectiveServiceV116();
    if(on_thread() && InterlockedCompareExchange(&active_flag,0,0) && !InterlockedCompareExchange(&fault,0,0)) {
        sync_epoch();game.cache=(uint64_t)(uintptr_t)cache;
        if(Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&sample)!=KH02_GATE_OK ||
           Kh02SessionObserve(&session,&sample)!=KH02_GATE_OK) {
            if(session.context || session.gate.ready)Kh02GateInvalidate(&session.gate);
            session.context=session.world=0;
        }
        if(session.context)menu_castle_auto161();
        if(session.context){Kh02StatReconcile();Kh02StatPopupServiceV111();Kh02CustomPopupServiceV115();Kh02MeterReconcile();movement_reconcile209();main_road_meter275();Kh02GoalProbeService();Kh02CombatBoostTick();Kh02PhantomPublish();}
        Kh02GoalObservationService();
        Kh02ObjectiveAutoV120();
        TideChoiceServiceV87();
    }
    InterlockedDecrement(&flight);
}
void kh02_original_route_v127(uint64_t *frame,uintptr_t caller) {
    if(GetCurrentThreadId()==owner_thread && caller==game.base+0x5ca61b)menu_token150.armed=0;
    Kh02OpeningTraceOriginal(game.base,caller,frame);
    exit_observe_v132(frame,caller);
}
void kh02_travel_capture(void *context,uint64_t map,uint64_t spawn,uint32_t fade,void *caller) {
    resource_begin211((uint64_t)(uintptr_t)context);
    resource_travel210((uint64_t)(uintptr_t)context,map,spawn,caller);
    Kh02OpeningTraceRoute(game.base,(uintptr_t)caller,(uintptr_t)context,map,spawn,fade);
    (void)context;(void)caller;Kh02GoalObservationGap();
    InterlockedIncrement(&flight);
    if(on_thread() && InterlockedCompareExchange(&active_flag,0,0)) {
        Kh02RoutePolicy policy={0};Kh02RouteRequest request={0};Kh02RouteResult result;
        policy.owner_cookie=request.owner_cookie=session.gate.current.cookie;
        policy.generation=request.generation=session.gate.current.generation;
        request.map=map;request.spawn=spawn;request.fade=fade;
        if(Kh02ObjectiveBeforeTransitionV116()) {
            int keep_rush200=local_rush_transition200((uint64_t)(uintptr_t)context,map,spawn,fade,(uint64_t)(uintptr_t)caller);
            Kh02SessionTravel(&session,&policy,&request,&result);
#if defined(KH02_GRANT_OWNER_LIVE) || defined(KH02_GRANT_OWNER_SELFTEST)
            local_travel200(keep_rush200);
#endif
        }
        game.cache=0;
    }
    InterlockedDecrement(&flight);
}
static void jump(unsigned char *p,void *destination) {
    uint64_t address=(uint64_t)(uintptr_t)destination;
    p[0]=0xff;p[1]=0x25;memset(p+2,0,4);memcpy(p+6,&address,8);
}
static uint32_t install(Entry *e,unsigned char *target,const unsigned char *prefix,
        size_t size,void *thunk,void **trampoline) {
    DWORD ignored;HMODULE pin;
    if(memcmp(target,prefix,size))return KH02_PATCH_CONFLICT;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        (LPCWSTR)(uintptr_t)thunk,&pin))return KH02_CLEANUP_UNCERTAIN;
    *trampoline=VirtualAlloc(NULL,size+14,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!*trampoline)return KH02_CLEANUP_UNCERTAIN;
    memcpy(*trampoline,prefix,size);jump((unsigned char*)*trampoline+size,target+size);
    if(!FlushInstructionCache(GetCurrentProcess(),*trampoline,size+14))return KH02_CLEANUP_UNCERTAIN;
    e->target=target;e->size=size;memcpy(e->original,prefix,size);
    memset(e->patch,0x90,size);jump(e->patch,thunk);
    if(!VirtualProtect(target,size,PAGE_EXECUTE_READWRITE,&e->protection))return KH02_CLEANUP_UNCERTAIN;
    e->owned=1; /* Ownership precedes the first overwritten byte. */
    memcpy(target,e->patch,size);
    if(!FlushInstructionCache(GetCurrentProcess(),target,size) ||
       !VirtualProtect(target,size,e->protection,&ignored))return KH02_CLEANUP_UNCERTAIN;
    return memcmp(target,e->patch,size)?KH02_PATCH_CONFLICT:KH02_OK;
}

#if defined(KH02_GRANT_OWNER_LIVE)
#include "xp_observer_v179.inc"
#endif
uint32_t Kh02XpConfigure185(uint64_t cookie,uint32_t halves) {
    if(GetCurrentThreadId()!=owner_thread || !active_flag || fault || !life ||
       !cookie || cookie!=session.gate.current.cookie ||
       (halves!=2 && halves!=3 && halves!=4 && halves!=6 && halves!=8) ||
       (xp_halves185 && xp_halves185!=halves))return 2;
    xp_halves185=halves;
#if defined(KH02_GRANT_OWNER_LIVE)
    if(xp_trace179)xp_trace179->reserved=(LONG)halves;
#endif
    return 1;
}
uint32_t Kh02GrantOwnerArm(uintptr_t base,Kh02Lifecycle *lifecycle,uint64_t cookie,uint64_t nonce) {
    IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS64 nt;unsigned char prefix[19];uint32_t status;
#if !defined(KH02_GRANT_OWNER_LIVE) && !defined(KH02_GRANT_OWNER_SELFTEST)
    (void)base;(void)lifecycle;(void)cookie;(void)nonce;
    return KH02_MEMBER_REFUSED;
#endif
    if(!lifecycle || !cookie || !nonce || life || session.gate.current.cookie || fault ||
       base<0x10000 || base>0x7fffffffffffULL-0x04f5f000)return KH02_MEMBER_REFUSED;
    if(!read_local(NULL,base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE ||
       dos.e_lfanew<0 || dos.e_lfanew>0x100000 ||
       !read_local(NULL,base+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       nt.OptionalHeader.SizeOfImage!=0x04f5f000)return KH02_BAD_BUILD;
    if(!read_local(NULL,base+0x438870,prefix,19) || memcmp(prefix,ww_remote_prefix,19) ||
       !read_local(NULL,base+0x459540,prefix,16) || memcmp(prefix,ww_close_prefix,16) ||
       !read_local(NULL,base+0x503760,prefix,15) || memcmp(prefix,magic_prefix,15) ||
       !read_local(NULL,base+0x587490,prefix,17) || memcmp(prefix,menu_prefix,17) ||
       !read_local(NULL,base+0x5115d0,prefix,15) || memcmp(prefix,magic_prefix,15) ||
       !read_local(NULL,base+0x511dc0,prefix,15) || memcmp(prefix,action_prefix,15) ||
       !read_local(NULL,base+0x511060,prefix,15) || memcmp(prefix,action_prefix,15) ||
       !read_local(NULL,base+0x512300,prefix,15) || memcmp(prefix,recovery_prefix,15) ||
       !read_local(NULL,base+0x517320,prefix,15) || memcmp(prefix,query_prefix,15) ||
       !read_local(NULL,base+0x458f40,prefix,18) || memcmp(prefix,travel_prefix,18) ||
       !read_local(NULL,base+0x5c2bc0,prefix,16) || memcmp(prefix,stat_prefix,16))return KH02_PATCH_CONFLICT;
    if(!read_local(NULL,base+0x85c700,prefix,15) || memcmp(prefix,zodiac_query_prefix,15))return KH02_PATCH_CONFLICT;
    if(!read_local(NULL,base+0x5bdf40,prefix,14) || memcmp(prefix,confirm_open_prefix,14) ||
       !read_local(NULL,base+0x5afe60,prefix,15) || memcmp(prefix,confirm_event_prefix,15) ||
       !read_local(NULL,base+0x5ac840,prefix,15) || memcmp(prefix,confirm_phase_prefix,15))return KH02_PATCH_CONFLICT;
    if(!read_local(NULL,base+0x446e10,prefix,14) || memcmp(prefix,stat_award_prefix_v108,14) ||
       !read_local(NULL,base+0x42bc30,prefix,17) || memcmp(prefix,stat_rebuild_prefix_v108,17))return KH02_PATCH_CONFLICT;
    if(!read_local(NULL,base+0x950350,prefix,15) || memcmp(prefix,wipeout_prefix_v143,15))return KH02_PATCH_CONFLICT;
    if(!read_local(NULL,base+0x57c120,prefix,15) || memcmp(prefix,menu_predicate_prefix151,15) ||
       !read_local(NULL,base+0x435c40,prefix,14) || memcmp(prefix,menu_discovery_prefix151,14))return KH02_PATCH_CONFLICT;
    if(!read_local(NULL,base+0x4c7710,prefix,15) || memcmp(prefix,mickey_prefix167,15))return KH02_PATCH_CONFLICT;
#ifdef KH02_GRANT_OWNER_LIVE
    if(!read_local(NULL,base+0x5d1b6a,prefix,18) || memcmp(prefix,menu_order_prefix213,18) ||
       !read_local(NULL,base+0x5d451c,prefix,14) || memcmp(prefix,menu_seen_prefix213,14))return KH02_PATCH_CONFLICT;
#endif
    if(!mickey_init167() || !mickey_control_init168())return KH02_CLEANUP_UNCERTAIN;
    if(!wipeout_init_v143())return KH02_CLEANUP_UNCERTAIN;
    if(!stat_trace_init_v108())return KH02_CLEANUP_UNCERTAIN;
    if(!confirm_init_v86())return KH02_CLEANUP_UNCERTAIN;
    if(!zodiac_observer_init())return KH02_CLEANUP_UNCERTAIN;
    #ifdef KH02_GRANT_OWNER_LIVE
    unsigned char xp_check179[41];
    if(!read_local(NULL,base+0x571560,prefix,15) || memcmp(prefix,broadcast_prefix192,15))return KH02_PATCH_CONFLICT;
    if(!broadcast_init192() || !local_init193())return KH02_CLEANUP_UNCERTAIN;
    if(!read_local(NULL,base+0x42b1f0,xp_check179,41) || memcmp(xp_check179,xp_leaf179,41))return KH02_PATCH_CONFLICT;
    if(!xp_init179())return KH02_CLEANUP_UNCERTAIN;
    #endif
    owner_thread=GetCurrentThreadId();life=lifecycle;sealed_nonce=nonce;
    if(Kh02SessionInit(&session,cookie)!=KH02_GATE_OK)return KH02_BUSY;
    game=(Kh02GameGrantContext){base,0,&session,read_local,NULL,is_active,NULL};
    seen_epoch=Kh02LifecycleEpoch(life);diagnostic_epoch();
    status=install(&entries[0],(unsigned char*)base+0x458f40,travel_prefix,18,kh02_travel_thunk,&kh02_travel_trampoline);
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(status==KH02_OK && test_fail_second)return KH02_CLEANUP_UNCERTAIN;
#endif
    if(status==KH02_OK)status=install(&entries[1],(unsigned char*)base+0x5c2bc0,stat_prefix,16,kh02_stat_thunk,&kh02_stat_trampoline);
    if(status==KH02_OK)status=install(&entries[2],(unsigned char*)base+0x512300,recovery_prefix,15,
        kh02_default_ability_thunk,&kh02_default_ability_trampoline);
    if(status==KH02_OK)status=install(&entries[3],(unsigned char*)base+0x517320,query_prefix,15,kh02_native_ability_thunk,&kh02_native_ability_trampoline);
    if(status==KH02_OK)status=install(&entries[4],(unsigned char*)base+0x511dc0,action_prefix,15,kh02_guard_thunk,&kh02_guard_trampoline);
    if(status==KH02_OK)status=install(&entries[5],(unsigned char*)base+0x511060,action_prefix,15,kh02_dodge_thunk,&kh02_dodge_trampoline);
    if(status==KH02_OK)status=install(&entries[6],(unsigned char*)base+0x5115d0,magic_prefix,15,kh02_magic_predicate_thunk,&kh02_magic_predicate_trampoline);
    if(status==KH02_OK)status=install(&entries[7],(unsigned char*)base+0x587490,menu_prefix,17,kh02_menu_thunk,&kh02_menu_trampoline);
    if(status==KH02_OK)status=install(&entries[8],(unsigned char*)base+0x503760,magic_prefix,15,kh02_style_request_thunk,&kh02_style_request_trampoline);
    if(status==KH02_OK)status=install(&entries[9],(unsigned char*)base+0x438870,ww_remote_prefix,19,kh02_ww_remote,&kh02_ww_remote_trampoline);
    if(status==KH02_OK)status=install(&entries[10],(unsigned char*)base+0x85c700,zodiac_query_prefix,15,zodiac_query_observe,&zodiac_query_trampoline);
    if(status==KH02_OK)status=install(&entries[11],(unsigned char*)base+0x5bdf40,confirm_open_prefix,14,confirm_open_v86,&confirm_open_trampoline);
    if(status==KH02_OK)status=install(&entries[12],(unsigned char*)base+0x5afe60,confirm_event_prefix,15,confirm_event_v86,&confirm_event_trampoline);
    if(status==KH02_OK)status=install(&entries[13],(unsigned char*)base+0x5ac840,confirm_phase_prefix,15,confirm_phase_v86,&confirm_phase_trampoline);
    if(status==KH02_OK)status=install(&entries[14],(unsigned char*)base+0x446e10,stat_award_prefix_v108,14,stat_award_observe_v108,&stat_award_trampoline_v108);
    if(status==KH02_OK)status=install(&entries[15],(unsigned char*)base+0x42bc30,stat_rebuild_prefix_v108,17,stat_rebuild_observe_v108,&stat_rebuild_trampoline_v108);
    if(status==KH02_OK)status=install(&entries[16],(unsigned char*)base+0x950350,wipeout_prefix_v143,15,wipeout_observe_v143,&wipeout_trampoline_v143);
    if(status==KH02_OK)status=install(&entries[17],(unsigned char*)base+0x57c120,menu_predicate_prefix151,15,menu_predicate151,&menu_predicate_trampoline151);
    if(status==KH02_OK)status=install(&entries[18],(unsigned char*)base+0x435c40,menu_discovery_prefix151,14,kh02_menu_discovery_thunk151,&menu_discovery_unused151);
    if(status==KH02_OK)status=install(&entries[19],(unsigned char*)base+0x4c7710,mickey_prefix167,15,mickey_observe167,&mickey_trampoline167);
    #ifdef KH02_GRANT_OWNER_LIVE
    if(status==KH02_OK)status=xp_install179(&entries[20],(unsigned char*)base+0x42b1f0);
    if(status==KH02_OK)status=install(&entries[21],(unsigned char*)base+0x571560,broadcast_prefix192,15,broadcast_observe192,&broadcast_trampoline192);
    if(status==KH02_OK)status=install(&entries[22],(unsigned char*)base+0x5d1b6a,menu_order_prefix213,18,kh02_menu_order_thunk213,&kh02_menu_order_trampoline213);
    kh02_menu_seen_resume213=(void*)(base+0x5d452a);
    if(status==KH02_OK)status=install(&entries[23],(unsigned char*)base+0x5d451c,menu_seen_prefix213,14,kh02_menu_seen_thunk213,&menu_seen_unused213);
    #endif
    if(status==KH02_OK && !death_init250())status=KH02_PATCH_CONFLICT;
#ifdef KH02_GRANT_OWNER_LIVE
    if(status==KH02_OK)status=install(&entries[24],(unsigned char*)base+0x519200,death_prefix250,18,death_observe250,&death_trampoline250);
#endif
    if(status==KH02_OK)InterlockedExchange(&active_flag,1);
    return status; /* Even a partial install is retained until process exit. */
}
static void outcome(Kh02GrantOutcome *out,uint64_t epoch,uint64_t nonce) {
    memset(out,0,sizeof(*out));out->epoch=epoch;out->nonce=nonce;
    out->status=KH02_MEMBER_REFUSED;out->before=out->after=UINT32_MAX;
    out->consumed=InterlockedCompareExchange(&attempted,0,0);
}
uint32_t Kh02GrantOwnerProbe(uint64_t epoch,Kh02GrantOutcome *out) {
    Kh02ReceiptGrant packet;Kh02SessionSample sample;
    if(!out)return KH02_MEMBER_REFUSED;outcome(out,epoch,sealed_nonce);
    InterlockedIncrement(&flight);
    if(is_active(NULL) && epoch==seen_epoch && !InterlockedCompareExchange(&busy,0,0) &&
       Kh02GameGrantPrepare(&game,Kh02LifecycleCurrent(life),1,&packet,&sample)==KH02_MEMBER_OK) {
        out->before=out->after=packet.before.counts[0];out->status=KH02_MEMBER_OK;
    }
    InterlockedDecrement(&flight);return out->status;
}
uint32_t Kh02GrantOwnerExecute(uint64_t epoch,uint64_t nonce,uint32_t expected,
    const unsigned char *proposal,uint32_t size,Kh02GrantOutcome *out) {
    Kh02ReceiptGrant packet;Kh02SessionSample sample;unsigned char owned[102456];uint32_t status=KH02_MEMBER_REFUSED;
    if(!out)return status;outcome(out,epoch,nonce);
    InterlockedIncrement(&flight);
    if(!on_thread() || !nonce || nonce!=sealed_nonce || InterlockedCompareExchange(&busy,1,0))goto done;
    if(InterlockedCompareExchange(&attempted,1,0))goto release;
    out->consumed=1; /* Permanently consumed before any reader/converter call. */
    if(expected>=99 || !proposal || size<56 || size>sizeof(owned))goto release;
    memcpy(owned,proposal,size);
    if(!is_active(NULL) || epoch!=seen_epoch || !Kh02GameGrantCallsEnabled() ||
       Kh02GameGrantPrepare(&game,Kh02LifecycleCurrent(life),1,&packet,&sample)!=KH02_MEMBER_OK)goto release;
    out->before=packet.before.counts[0];
    if(out->before!=expected)goto release;
    status=Kh02LifecycleGrant(life,epoch,1,&sample,owned,size,&packet);
    out->entered=packet.entered;out->before=packet.observed_before;out->after=packet.observed_after;
release:InterlockedExchange(&busy,0);
done:out->status=status;out->consumed=InterlockedCompareExchange(&attempted,0,0);
    InterlockedDecrement(&flight);return status;
}
uint32_t Kh02GrantOwnerStop(void) {
    InterlockedExchange(&active_flag,0);
    if(!owner_thread)return KH02_OK;
    if(!on_thread())return KH02_BUSY;
    if(!Kh02ObjectiveBeforeTransitionV116())return KH02_CLEANUP_UNCERTAIN;
    Kh02SessionStop(&session);
    return InterlockedCompareExchange(&flight,0,0)?KH02_BUSY:KH02_OK;
}
uint32_t Kh02GrantOwnerFlight(void){return InterlockedCompareExchange(&flight,0,0);}
