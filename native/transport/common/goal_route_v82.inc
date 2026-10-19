/* Sole travel thunk: 1 refuses the call, 0 forwards the possibly redirected
 * frame. Only the audited generic-trigger caller may be refused. */
#include "tide_access_v77.h"
uint32_t Kh02AcceptedGoalAccessV80(uint32_t *out);
static Kh02TideAccessV77 goal_names;
static uint64_t goal_route_counts[8];
static uint32_t goal_route_last;
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02GoalRouteDebugV82(uint64_t *out) {
    memcpy(out,goal_route_counts,sizeof(goal_route_counts));out[8]=goal_route_last;
}
#endif
static uint32_t goal_route_report(uintptr_t caller,uint64_t owner,uint32_t reason,uint32_t refuse) {
    goal_route_last=reason;goal_route_counts[reason]++;
    Kh02OpeningTraceStart(game.base,caller,owner,82,reason);
    return refuse;
}
/* Names are process-local immutable identifiers, not load-scoped ownership.
 * Resolve once on the owner thread; never accept a partial cache. */
static int goal_route_names(void) {
    Kh02TideAccessV77 next={0};
    if(goal_names.tide_map)return 1;
    next.tide_map=start_name(L"/Game/Levels/dw/dw_10/dw_10",1);
    next.tide_spawn=start_name(L"dw_10_Lv_Start_01",1);
    next.depths_map=start_name(L"/Game/Levels/dw/dw_09/dw_09",1);
    next.save_spawn=start_name(L"dw_09_Lv_Save_01",1);
    if(!next.tide_map || !next.tide_spawn || !next.depths_map || !next.save_spawn ||
       next.tide_map==next.depths_map)return 0;
    goal_names=next;return 1;
}
/* 1 is the exact supported final source, 2 is a verified different receiver,
 * 0 is uncertain. Uncertain is never interpreted as permission to end a run. */
static int goal_route_context(uint64_t context) {
    uint64_t root,instance,current,table;char map[32];uint32_t story;
    if(!context || (context&7) ||
       !read_local(NULL,game.base+0x480b5d0,&root,8) || !root ||
       !read_local(NULL,root+0x230,&instance,8) || !instance ||
       !read_local(NULL,instance+0x50,&current,8) || !current)return 0;
    if(current!=context)return 2;
    if(!read_local(NULL,context,&table,8) || table!=game.base+0x36df228 ||
       !read_local(NULL,context+0x384c,map,sizeof(map)) ||
       memcmp(map,"/Game/Levels/dw/dw_09/dw_09",sizeof("/Game/Levels/dw/dw_09/dw_09")) ||
       !read_local(NULL,context+0x3338,&story,4) || story!=410)return 0;
    return 1;
}
uint32_t kh02_goal_route_filter(uint64_t *frame,uintptr_t caller) {
    Kh02TideAccessV77 a;Kh02RouteRequest q={0};Kh02RouteResult result;
    uint32_t mask=0,decision;uint64_t epoch;int context;
    if(!frame || caller!=game.base+0x563c95)return 0;
    /* Without known names, an unrelated generic transition cannot safely be
     * distinguished. Refuse with evidence rather than guess a destination. */
    if(GetCurrentThreadId()!=owner_thread)
        return goal_route_report(caller,frame[0],0,1);
    if(!goal_route_names())return goal_route_report(caller,frame[0],1,1);
    if(frame[1]!=goal_names.tide_map || frame[2]!=goal_names.tide_spawn)
        return goal_route_report(caller,frame[0],2,0);
    context=goal_route_context(frame[0]);
    if(context==2)return goal_route_report(caller,frame[0],3,0);
    if(context!=1)return goal_route_report(caller,frame[0],4,1);
    a=goal_names;result.map=a.depths_map;result.spawn=a.save_spawn;
    epoch=Kh02LifecycleEpoch(life);
    if(active_flag && !fault && !busy && session.context==frame[0] &&
       session.gate.current.cookie && epoch && Kh02AcceptedGoalAccessV80(&mask)) {
        a.cookie=session.gate.current.cookie;a.epoch=epoch;a.owner=frame[0];
        a.admitted=1;a.unlocked=mask&1;
        q.owner_cookie=a.cookie;q.generation=epoch;q.map=frame[1];q.spawn=frame[2];
        q.caller_rva=0x563c95;q.fade=(uint32_t)frame[3];
        decision=Kh02TideAccessDecideV77(&a,frame[0],epoch,
            "/Game/Levels/dw/dw_09/dw_09",410,&q,&result);
        if(decision!=KH02_ALLOW && decision!=KH02_REDIRECT)
            return goal_route_report(caller,frame[0],5,1);
        depths_complete_v145(frame[0],epoch,a.cookie);
    }
    if(result.map==a.tide_map) {
        uint32_t choice=tide_choice_begin(frame,epoch);
        if(choice==2)return 1; /* Existing pending dialog, unchanged refusal. */
        if(choice==1)return goal_route_report(caller,frame[0],7,1);
        result.map=a.depths_map;result.spawn=a.save_spawn;
    }
    frame[1]=result.map;frame[2]=result.spawn;
    return goal_route_report(caller,frame[0],6,0);
}
