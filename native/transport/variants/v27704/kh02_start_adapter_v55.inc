/* Development Forest-only fixture. Single existing travel-hook owner.
 * New timing remains live-unproven; legacy three-copy implementation retained. */
#include "kh02_start_plan_v55.h"
#include "kh02_restore_snapshot_candidate.h"
static Kh02StartPlan start_plan;
static uint32_t ww_pending,ww_other_events;
/* Owner-thread-only marker for the existing forwarded remote-event call. */
static uint32_t ww_forward_depth_v134;
static volatile LONG ww_raw_admitted,ww_raw_thread,ww_raw_idle;
uint32_t Kh02ControlledStartAuthorized(void);
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02StartDebugV56(uint64_t *out) {
    uint64_t v[]={start_plan.phase,start_plan.owner,start_plan.cookie,start_plan.epoch,
        start_plan.thread,owner_thread,Kh02LifecycleEpoch(life),active_flag,fault,busy,
        Kh02ControlledStartAuthorized()};memcpy(out,v,sizeof(v));
}
#endif
void Kh02StartFreshCandidate(uintptr_t context) {
    if(GetCurrentThreadId()==owner_thread && active_flag && !fault)
        (void)Kh02StartBind(&start_plan,context,session.gate.current.cookie,1,owner_thread);
    Kh02OpeningTraceStart(game.base,game.base+0x56e280,context,start_plan.phase,1);
}
static uint64_t start_name(const wchar_t *text,int add) {
    typedef uint64_t *(__fastcall *Fn)(uint64_t*,const wchar_t*,int);
    static const unsigned char prefix[]={0x40,0x53,0x48,0x83,0xec,0x30,0x48,0x8b,0xd9,0x48,0x85,0xd2,0x74,0x21,0x45,0x8b};
    unsigned char bytes[16];uint64_t value=0;
    if(!read_local(NULL,game.base+0xb76a10,bytes,16) || memcmp(bytes,prefix,16))return 0;
    return ((Fn)(game.base+0xb76a10))(&value,text,add)==&value?value:0;
}
#include "goal_route_v82.inc"
static int start_records(uint64_t owner,uint32_t *a,uint32_t *b) {
    Kh02RestoreSnapshot snapshot;char ma[32],mb[32];
    static const char expected[]="/Game/Levels/dw/dw_58/dw_58";
    if(Kh02RestoreSnapshotRead(read_local,NULL,game.base,owner,&snapshot)!=KH02_RESTORE_OK)return 0;
    return read_local(NULL,owner+0x3338,a,4) && read_local(NULL,owner+0x7060,b,4) &&
        read_local(NULL,owner+0x384c,ma,sizeof(ma)) && read_local(NULL,owner+0x7574,mb,sizeof(mb)) &&
        !memcmp(ma,expected,sizeof(expected)) && !memcmp(mb,expected,sizeof(expected));
}
static int start_write(uint64_t address,uint32_t value) {
    SIZE_T n=0;
    return WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)address,&value,4,&n) && n==4;
}
/* Native459230 obtains spawn text through433A40 from map metadata+100.
 * Read only that fixed256-byte field in each admitted structured record.
 * No inferred spawn spelling or cross-process numeric FName is accepted. */
static uint64_t start_intro_spawn(uint64_t owner) {
    unsigned char a[256],b[256];wchar_t text[256];unsigned i;
    if(!read_local(NULL,owner+0x394c,a,sizeof(a)) ||
       !read_local(NULL,owner+0x7674,b,sizeof(b)))return 0;
    for(i=0;i<256;i++) {
        if(a[i]!=b[i] || a[i]>127)return 0;
        text[i]=(wchar_t)a[i];
        if(!a[i])return i?start_name(text,0):0;
    }
    return 0;
}
/* frame holds original RCX,RDX,R8,R9 saved by the existing assembly thunk. */
void kh02_start_route_adapt(uint64_t *frame,uintptr_t caller) {
    Kh02StartRequest q={0};Kh02StartDecision d;uint32_t a,b;
    if(!frame || !start_plan.phase || start_plan.phase==3)return;
    q.owner=frame[0];q.map=frame[1];q.spawn=frame[2];q.thread=GetCurrentThreadId();
    q.cookie=session.gate.current.cookie;q.epoch=Kh02LifecycleEpoch(life);
    q.caller=caller>=game.base?(uint32_t)(caller-game.base):0;
    q.authorized=q.thread==owner_thread && active_flag && !fault && !busy &&
        Kh02ControlledStartAuthorized()==1 && Kh02LifecycleCurrent(life)!=NULL;
    if(!q.authorized || q.owner!=start_plan.owner || q.epoch!=1) {start_plan.phase=3;Kh02OpeningTraceStart(game.base,caller,q.owner,3,2);return;}
    q.records_verified=start_records(q.owner,&q.primary_story,&q.checkpoint_story);
    if(!q.records_verified || q.primary_story || q.checkpoint_story) {start_plan.phase=3;Kh02OpeningTraceStart(game.base,caller,q.owner,3,3);return;}
    if(start_plan.phase==1) {
        q.intro_map=start_name(L"/Game/Levels/dw/dw_58/dw_58",0);
        q.intro_spawn=start_intro_spawn(q.owner);
    } else {
        q.road_map=start_name(L"/Game/Levels/dw/dw_01/dw_01",0);
        q.road_spawn=start_name(L"dw_01_Lv_Start_01",0);
        q.depths_map=start_name(L"/Game/Levels/dw/dw_09/dw_09",1);
        q.depths_spawn=start_name(L"dw_09_Lv_Start_01",1);
    }
    if(!Kh02StartDecide(&start_plan,&q,&d)){Kh02OpeningTraceStart(game.base,caller,q.owner,start_plan.phase,4);return;}
    /* Revalidate after engine FName calls, before touching the fixed two words. */
    if(!Kh02ControlledStartAuthorized() || !Kh02LifecycleCurrent(life) ||
       Kh02LifecycleEpoch(life)!=1 || !start_records(q.owner,&a,&b) || a || b){Kh02OpeningTraceStart(game.base,caller,q.owner,3,5);return;}
    Kh02OpeningTraceRoute(game.base,caller,q.owner,q.map,q.spawn,(uint32_t)frame[3]);
    if(start_write(q.owner+0x3338,360) && start_write(q.owner+0x7060,360) &&
       start_records(q.owner,&a,&b) && a==360 && b==360) {
        frame[1]=d.map;frame[2]=d.spawn;Kh02OpeningTraceStart(game.base,caller,q.owner,3,6);return;
    }
    /* Failed staging retains original route. Restore both fixed words and fault
       the owner if rollback cannot be verified; never retry a consumed token. */
    int restored_a=start_write(q.owner+0x3338,0);
    int restored_b=start_write(q.owner+0x7060,0);
    if(!restored_a || !restored_b || !start_records(q.owner,&a,&b) || a || b) {
        InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);
    }
    Kh02OpeningTraceStart(game.base,caller,q.owner,3,fault?8:7);
}

/* Reuse the legacy exact-event close BEFORE forwarding the native broadcast.
 * Scoped to this authenticated fresh redirect, never a general loading fix. */
static uint8_t __fastcall kh02_ww_remote(void *context,uint64_t event) {
    typedef uint8_t (__fastcall *RemoteFn)(void*,uint64_t);
    typedef void (__fastcall *CloseFn)(void);
    uint8_t result;uint32_t story=0;uint64_t expected,root,instance,current;Kh02RestoreSnapshot snapshot;
    unsigned char prefix[16];
    InterlockedIncrement(&flight);
    {
        uint32_t reason=ww_pending?(GetCurrentThreadId()==owner_thread?30u:31u):32u;
        volatile LONG *counter=reason==30?&ww_raw_admitted:reason==31?&ww_raw_thread:&ww_raw_idle;
        if(InterlockedIncrement(counter)<=8)
            Kh02OpeningTraceRemote(game.base,0,(uintptr_t)context,event,reason);
    }
    rush_event162(context,event);
    menu_ww_event150(context,event);
    if(!ww_pending || GetCurrentThreadId()!=owner_thread)goto original;
    if(!active_flag || fault || busy || Kh02LifecycleEpoch(life)!=1 ||
       !Kh02ControlledStartAuthorized() || session.gate.current.cookie!=start_plan.cookie) {
        ww_pending=0;Kh02OpeningTraceStart(game.base,game.base+0x438870,start_plan.owner,3,21);goto original;
    }
    expected=start_name(L"dw_50_gameflow_Evt_cutscene_dw50_dw015_Movie",0);
    if(!expected || expected!=event) {
        if(ww_other_events++<8)Kh02OpeningTraceRemote(game.base,start_plan.owner,(uintptr_t)context,event,22);
        goto original;
    }
    ww_pending=0; /* Consume before the native call, including reentrant events. */
    if(Kh02RestoreSnapshotRead(read_local,NULL,game.base,start_plan.owner,&snapshot)!=KH02_RESTORE_OK ||
       !read_local(NULL,start_plan.owner+0x3338,&story,4) || story!=200 ||
       !read_local(NULL,game.base+0x480b5d0,&root,8) || !root ||
       !read_local(NULL,root+0x230,&instance,8) || !instance ||
       !read_local(NULL,instance+0x50,&current,8) || current!=start_plan.owner ||
       !read_local(NULL,game.base+0x459540,prefix,16) || memcmp(prefix,ww_close_prefix,16)) {
        Kh02OpeningTraceRemote(game.base,start_plan.owner,(uintptr_t)context,event,23);goto original;
    }
    Kh02OpeningTraceRemote(game.base,start_plan.owner,(uintptr_t)context,event,24);
    ((CloseFn)(game.base+0x459540))();
    Kh02OpeningTraceRemote(game.base,start_plan.owner,(uintptr_t)context,event,25);
original:
    if(GetCurrentThreadId()==owner_thread)ww_forward_depth_v134++;
    result=((RemoteFn)kh02_ww_remote_trampoline)(context,event);
    if(GetCurrentThreadId()==owner_thread)ww_forward_depth_v134--;
    InterlockedDecrement(&flight);return result;
}
