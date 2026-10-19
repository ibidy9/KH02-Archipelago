/* Fixed map identities only. Resolve through the game's item manager before
 * entering the existing cosmetic receipt transaction; never guess an item. */
#include "kh02_area_map_v101.h"
static uint32_t area_map_current(Kh02GameGrantContext *p) {
    return p && GetCurrentThreadId()==owner_thread && p->session==&session &&
        p->base==game.base && p->cache==game.cache && p->read==game.read &&
        p->read_context==game.read_context && p->active && p->active(p->active_context)==1 &&
        Kh02ControlledStartAuthorized() && is_active(NULL) &&
        !session.stopped && !session.uncertain && session.gate.current.observation && !session.gate.busy;
}
uint32_t Kh02AreaMapVerifyCode(Kh02GameGrantContext *p,uint64_t item) {
    static const wchar_t *keys[]={L"NAVI_MAP_DW01",L"NAVI_MAP_DW02",L"NAVI_MAP_DW03",L"NAVI_MAP_DW04"};
    static const unsigned char prefix[]={0x48,0x83,0x79,0x40,0,0x4c,0x8b,0xd1};
    Kh02AreaMapDescriptor descriptor;Kh02Lease lease;
    uint64_t manager,name,context,epoch;uint32_t code;unsigned char bytes[sizeof(prefix)];
    if(!Kh02AreaMapResolve(item,&descriptor) || !area_map_current(p))return 0;
    lease=session.gate.current;context=session.context;epoch=Kh02LifecycleEpoch(life);
    if(!read_local(NULL,p->base+0x4841f0,bytes,sizeof(bytes)) || memcmp(bytes,prefix,sizeof(bytes)) ||
       !read_local(NULL,context+0x7e08,&manager,8) || manager<0x10000 ||
       manager>0x7fffffffffffULL-0x200 || (manager&7))return 0;
    name=start_name(keys[item-0x02022f],0);
    if(!name || !area_map_current(p) || session.context!=context ||
       epoch!=Kh02LifecycleEpoch(life) || memcmp(&lease,&session.gate.current,sizeof(lease)))return 0;
    code=((uint32_t (__fastcall *)(void*,uint64_t))(p->base+0x4841f0))((void*)(uintptr_t)manager,name);
    return code==descriptor.code && area_map_current(p) && session.context==context &&
        epoch==Kh02LifecycleEpoch(life) && !memcmp(&lease,&session.gate.current,sizeof(lease));
}
