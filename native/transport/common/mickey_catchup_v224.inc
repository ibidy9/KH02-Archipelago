/* Development-only, explicit ten-minute lease, maximum eight native recalls. */
#include <math.h>
#include "mickey_guard_v224.inc"
typedef struct AutoPolicy224 {uint64_t last,far_since,entry,combat,cooldown;int valid,battle;} AutoPolicy224;
static AutoPolicy224 ap224;
#ifdef KH02_GRANT_OWNER_SELFTEST
static uint64_t test_now224;
static int test_pause224=-1;
KH02_RECEIPT_API void Kh02CatchupTest224(uint64_t now,int paused){test_now224=now;test_pause224=paused;}
#endif
static uint64_t now224(void) {
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(test_now224)return test_now224;
#endif
    return GetTickCount64();
}
static void auto_reset224(void){uint64_t cooldown=ap224.cooldown;memset(&ap224,0,sizeof(ap224));ap224.cooldown=cooldown;}
static uint32_t auto_policy224(uint64_t now,double d,int battle,int safe) {
    uint32_t reason=0;
    if(!safe || !isfinite(d) || d<0) {auto_reset224();return 0;}
    if(!ap224.valid || now<ap224.last || now-ap224.last>2000) {
        auto_reset224();ap224.valid=1;ap224.last=now;ap224.battle=battle;return 0;
    }
    if(battle && !ap224.battle){ap224.far_since=0;ap224.entry=d>1200?now:0;}
    ap224.battle=battle;ap224.last=now;
    if(!battle) {
        ap224.entry=ap224.combat=0;
        if(d>2000){if(!ap224.far_since)ap224.far_since=now;}else ap224.far_since=0;
    } else {
        ap224.far_since=0;
        if(d<=1200)ap224.entry=0;
        if(d>6000){if(!ap224.combat)ap224.combat=now;}else ap224.combat=0;
    }
    if(now<ap224.cooldown)return 0;
    if(ap224.entry && now-ap224.entry>=3000)reason=2;
    if(ap224.combat && now-ap224.combat>=5000)reason=3;
    if(ap224.far_since && now-ap224.far_since>=3000)reason=1;
    if(reason){ap224.cooldown=now+15000;ap224.far_since=ap224.entry=ap224.combat=0;}
    return reason;
}
static int auto_bytes224(uint64_t at,const unsigned char *expected,uint32_t size) {
    unsigned char bytes[256];return size<=sizeof(bytes)&&read_local(NULL,at,bytes,size)&&!memcmp(bytes,expected,size);
}
typedef struct AutoSample224 {uint64_t world,owner,player,manager,handle,pawn;double distance;uint32_t battle;} AutoSample224;
static int auto_sample224(AutoSample224 *out) {
    Kh02SessionSample s;MickeyRow167 r={0};uint32_t owned,hp,flags,player_flags,count,object_flags,serial;
    uint64_t root,table,item,player_root,friend_root,vt;int32_t index;float a[3],b[3];double d=0;
    memset(out,0,sizeof(*out));
    if(!Kh02ControlledStartAuthorized() || !is_active(NULL) || !session.context ||
       !Kh02MickeyOwnership171(&owned) || !owned || (mickey_mode168!=1 && mickey_mode168!=2) ||
       mickey_cookie168!=session.gate.current.cookie ||
       Kh02LifecycleEpoch(life)!=mickey_lab_epoch221 ||
       Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&s)!=KH02_GATE_OK ||
       s.context!=session.context || s.world!=session.world ||
       s.provider!=session.gate.current.provider || s.attributes!=session.gate.current.attributes ||
       !read_local(NULL,s.provider+0x9e8,&r.manager,8))return 0;
    r.thread=GetCurrentThreadId();mickey_before167(&r);
    if(r.reason!=5 || r.player!=s.provider || r.owner!=s.context || r.before!=1 ||
       r.manager!=mickey_lab_manager221 || (!lab_map225(r.map) || memcmp(r.map,mickey_lab_map225,32)) ||
       !read_local(NULL,r.manager+0x218,&out->handle,8) || out->handle!=mickey_lab_handle221 ||
       !read_local(NULL,r.player+0x5dc,&hp,4) || !hp ||
       !read_local(NULL,r.owner+0x348,&flags,4) || (flags&2) ||
       !read_local(NULL,r.player,&vt,8))return 0;
    {uint64_t target;
     if(!read_local(NULL,vt+0x13e8,&target,8)||target!=game.base+0x31f030)return 0;}
    root=s.world;
    if(!mickey_ptr167(root) || s.phase!=3 || s.title_flags || s.pending_map || (s.travel_flags&1) || s.world_flag ||
       !auto_bytes224(game.base+0x1eb3590,pause224,sizeof(pause224)) ||
       !auto_bytes224(game.base+0x4c5b10,warp224,sizeof(warp224)))return 0;
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(test_pause224>=0){if(test_pause224)return 0;}else
#endif
    if(((unsigned char (__fastcall *)(void*))(game.base+0x1eb3590))((void*)(uintptr_t)root))return 0;
    if(!read_local(NULL,r.owner+0x354,&flags,4) || !read_local(NULL,r.player+0x5fc,&player_flags,4) ||
       (!!(flags&5)!=!!(player_flags&2)))return 0;
    index=(int32_t)out->handle;serial=(uint32_t)(out->handle>>32);
    if(index<0 || !serial || !read_local(NULL,game.base+0x4498bdc,&count,4) ||
       count>10000000 || (uint32_t)index>=count ||
       !read_local(NULL,game.base+0x4498bd0,&table,8) || !mickey_ptr167(table))return 0;
    item=table+(uint64_t)index*24;
    {uint32_t actual;
     if(!read_local(NULL,item+16,&actual,4)||actual!=serial ||
        !read_local(NULL,item+8,&object_flags,4)||(object_flags&0x30000000)||
        !read_local(NULL,item,&out->pawn,8)||!mickey_ptr167(out->pawn))return 0;}
    if(!read_local(NULL,r.player+0x188,&player_root,8)||!mickey_ptr167(player_root)||
       !read_local(NULL,out->pawn+0x188,&friend_root,8)||!mickey_ptr167(friend_root)||
       !read_local(NULL,player_root+0x2b0,a,12)||!read_local(NULL,friend_root+0x2b0,b,12))return 0;
    for(unsigned i=0;i<3;i++) {double delta=(double)a[i]-b[i];
        if(!isfinite(a[i])||!isfinite(b[i])||fabs(a[i])>=10000000||fabs(b[i])>=10000000)return 0;
        d+=delta*delta;}
    out->world=root;out->owner=r.owner;out->player=r.player;out->manager=r.manager;
    out->distance=sqrt(d);out->battle=!!(flags&5);return 1;
}
static void catchup224(void) {
    AutoSample224 a,b;uint32_t reason;uint64_t now;
    if(!auto_enabled224)return;
    {char map[32];
     if(!session.context || !read_local(NULL,session.context+0x384c,map,32) ||
        memcmp(map,mickey_lab_map225,32)) {auto_enabled224=0;auto_reset224();return;}}
    now=now224();
    if((mickey_mode168!=2 && (now>=auto_deadline224 || auto_count224>=8)) || Kh02LifecycleEpoch(life)!=mickey_lab_epoch221) {
        auto_enabled224=0;auto_reset224();return;
    }
    if(GetCurrentThreadId()!=owner_thread || InterlockedCompareExchange(&busy,1,0))return;
    if(!auto_sample224(&a)) {auto_reset224();goto done;}
    reason=auto_policy224(now,a.distance,a.battle,1);
    if(!reason)goto done;
    if(!auto_sample224(&b) || a.owner!=b.owner || a.player!=b.player || a.manager!=b.manager ||
       a.handle!=b.handle || a.pawn!=b.pawn || a.world!=b.world || a.battle!=b.battle ||
       b.distance<=(reason==1?2000:reason==2?1200:6000)) {auto_reset224();goto done;}
    /* Count/cooldown consumed before native call. No retry on uncertain return. */
    if(auto_count224<UINT32_MAX)auto_count224++;auto_reason224=reason;
    ((void (__fastcall *)(void*,unsigned char))(game.base+0x4c5b10))((void*)(uintptr_t)b.manager,0);
    {uint32_t count=0;uint64_t handle=0;
     if(!read_local(NULL,b.manager+0x238,&count,4)||count!=1||
        !read_local(NULL,b.manager+0x218,&handle,8)||handle!=b.handle)auto_enabled224=0;}
done:
    InterlockedExchange(&busy,0);
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API uint32_t Kh02CatchupPolicy224(uint64_t t,double d,int battle,int safe,int reset) {
    if(reset)memset(&ap224,0,sizeof(ap224));return auto_policy224(t,d,battle,safe);
}
#endif

