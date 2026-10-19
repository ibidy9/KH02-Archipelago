/* Main Road and pre-gear Castle native initialization; existing spell gates remain authoritative.
 * The reset was observed with magic bit4 absent and Focus0 on a new actor.
 * Never initialize a normal enabled/empty meter or refill it every tick. */
#include "meter_guards_v275.h"
#ifndef KH02_METER_ENABLE275
#define KH02_METER_ENABLE275(rva,controller) ((void (*)(void*,unsigned char))(uintptr_t)(game.base+(rva)))((void*)(uintptr_t)(controller),1)
#endif
static uint64_t meter_actor275,meter_epoch275;
static uint32_t meter_done275;
static void main_road_meter275(void) {
    Kh02SessionSample s;uint64_t epoch,controller,vt,fn,player,attrs;
    uint32_t magic=0,movement=0,focus=0,maxfocus=0;unsigned char bits,bytes[279];char map[64];
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL))return;
    epoch=Kh02LifecycleEpoch(life);
    if(!popup_sample_v111(&s,epoch) || !popup_ready273(read_local,NULL,game.base,s.context) ||
       !Kh02MagicOwnership(&magic) || !Kh02MovementOwnership209(&movement) ||
       !read_local(NULL,s.context+0x384c,map,64) ||
       (memcmp(map,"/Game/Levels/dw/dw_01/dw_01",28) &&
        memcmp(map,"/Game/Levels/dw/dw_02/dw_02",28)) ||
       !read_local(NULL,s.context+0x28ac,&bits,1) || (bits&4))return;
    if(!read_local(NULL,s.provider+0x4c8,&attrs,8) || attrs!=s.attributes ||
       !read_local(NULL,game.cache+0x180,&player,8) ||
       !read_local(NULL,player+0x98,&controller,8) ||
       !popup_vtable_v111(controller,&vt) ||
       !read_local(NULL,controller+0x3e8,&player,8) || player!=s.provider ||
       !read_local(NULL,vt+0x12d0,&fn,8) || fn!=game.base+0x4eef90 ||
       !read_local(NULL,vt+0x12e0,&fn,8) || fn!=game.base+0x4ef0b0 ||
       !read_local(NULL,s.provider+0x5f4,&focus,4) ||
       !read_local(NULL,s.attributes+0x58,&maxfocus,4) || focus>maxfocus || maxfocus!=100)return;
    for(uint32_t i=0;i<2;i++)if(!read_local(NULL,game.base+meter_guards275[i].rva,bytes,meter_guards275[i].size) ||
        memcmp(bytes,meter_guards275[i].bytes,meter_guards275[i].size))return;
    if(meter_actor275!=s.provider || meter_epoch275!=epoch){meter_actor275=s.provider;meter_epoch275=epoch;meter_done275=0;}
    uint32_t todo=((magic?4u:0u)|((movement&16)&&!focus?16u:0u))&~meter_done275;
    if(!todo || !popup_same_v111(&s,epoch) || InterlockedCompareExchange(&busy,1,0))return;
    /* Mark attempted before native calls; uncertainty must never produce a retry refill. */
    meter_done275|=todo;
    if(todo&16)KH02_METER_ENABLE275(0x4ef0b0,controller);
    if((todo&4) && popup_same_v111(&s,epoch))KH02_METER_ENABLE275(0x4eef90,controller);
    InterlockedExchange(&busy,0);
}
