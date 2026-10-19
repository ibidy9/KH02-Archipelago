/* Fixed refresh event, current shared owner, every world; no second hook. */
#include "kh02_zodiac_catalog_v99.h"
static uint32_t zodiac_refresh_current(Kh02GameGrantContext *p) {
    return p && GetCurrentThreadId()==owner_thread && p->session==&session &&
        p->base==game.base && p->cache==game.cache && p->read==game.read &&
        p->read_context==game.read_context && p->active && p->active(p->active_context)==1 &&
        Kh02ControlledStartAuthorized() && is_active(NULL) &&
        entries[9].owned && entries[9].target==(unsigned char*)(uintptr_t)(game.base+0x438870) &&
        !memcmp(entries[9].target,entries[9].patch,entries[9].size);
}
uint64_t Kh02ZodiacPrepareRefresh(Kh02GameGrantContext *p) {
    uint64_t event;
    if(!zodiac_refresh_current(p))return 0;
    event=start_name(L"UpdataStarRelief",0);
    if(!event)event=start_name(L"UpdataStarRelief",1);
    return zodiac_refresh_current(p)?event:0;
}
uint32_t Kh02ZodiacRefresh(Kh02GameGrantContext *p,uint64_t event) {
    if(!event || !zodiac_refresh_current(p) || !p->session->gate.busy)return 0;
    /* The return byte says whether a local event listener exists, not whether
       the owned Zodiac was registered. Forest may legitimately have none. */
    ((uint8_t (__fastcall *)(void*,uint64_t))(p->base+0x438870))(
        (void*)(uintptr_t)p->session->context,event);
    return zodiac_refresh_current(p);
}
