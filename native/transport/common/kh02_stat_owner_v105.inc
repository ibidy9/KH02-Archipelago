/* Fixed current-owner stat fields only; no new hook and no external writer. */
#include "kh02_stat_projection_v105.h"
#include "resource_trace_v210.inc"
extern uint32_t Kh02StatOwnership(uint32_t*,uint32_t*);
static uint32_t stat_hp,stat_mp;
static int stat_read(void *unused,Kh02StatView *out) {
    Kh02SessionSample s,again;uint64_t sub;uint32_t current[3];(void)unused;
    memset(out,0,sizeof(*out));
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) || !Kh02ControlledStartAuthorized() ||
       Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&s)!=KH02_GATE_OK ||
       s.phase!=3 || s.title_flags || s.pending_map || (s.travel_flags&1) || s.world_flag ||
       s.context!=session.context || s.world!=session.world || s.attributes!=session.gate.current.attributes)return 0;
    out->owner=s.context;out->attributes=s.attributes;out->epoch=Kh02LifecycleEpoch(life);
    if(!out->epoch || !read_local(NULL,s.context+0x538,&out->difficulty,4) ||
       !read_local(NULL,s.attributes+0x70,out->base,8) ||
       !read_local(NULL,s.attributes+0x50,out->maximum,8) ||
       !read_local(NULL,s.attributes+0xf8,&sub,8) || sub!=s.context+0x2930 ||
       !read_local(NULL,sub+0x64,out->bonuses,32) ||
       !read_local(NULL,s.provider+0x5dc,current,12))return 0;
    out->current[0]=current[0];out->current[1]=current[2];
    return is_active(NULL) && out->epoch==Kh02LifecycleEpoch(life) &&
        Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&again)==KH02_GATE_OK &&
        !memcmp(&s,&again,sizeof(s));
}
static int stat_write(void *unused,const Kh02StatView *before,const uint32_t *target) {
    Kh02StatView current;SIZE_T wrote=0;(void)unused;
    if(!stat_read(NULL,&current) || memcmp(before,&current,sizeof(current)))return 0;
    return WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)(before->attributes+0x50),target,8,&wrote) && wrote==8;
}
#include "resource_carry_v211.inc"
uint32_t Kh02StatReconcile(void) {
    uint32_t hp,mp,status;Kh02StatProjection result;
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) || !Kh02StatOwnership(&hp,&mp))return 0;
    if(!hp && !mp && !stat_hp && !stat_mp)return 1;
    if(InterlockedCompareExchange(&busy,1,0))return 0;
    status=Kh02StatProject(stat_read,stat_write,NULL,hp,mp,stat_hp,stat_mp,&result);
    /* v275: publish refusals as well; zero before.owner previously hid them. */
    if(!result.before.owner){ResourceRow210 row={0};row.kind=5;row.status=status;
        row.hp_items=hp;row.mp_items=mp;resource_record210(&row);}
    resource_projection210(&result,status,hp,mp);
    status=resource_finish211(&result,status,hp,mp);
    if(status==1){stat_hp=hp;stat_mp=mp;}
    else if(status==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
    InterlockedExchange(&busy,0);return status==1;
}
uint32_t Kh02StatReceiptReady(uint64_t item) {
    return (item==0x02020b || item==0x02020c) && Kh02StatReconcile();
}
