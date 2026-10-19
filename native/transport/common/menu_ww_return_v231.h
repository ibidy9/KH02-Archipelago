#include "menu_switch_pair_v232.h"
#ifndef KH02_MENU_WW_RETURN_V231_H
#define KH02_MENU_WW_RETURN_V231_H
#include "menu_forest_v149.h"
/* Ordinary earned WW hub return only, not restart or a mirror route.
 * Reconstruct a hub milestone from serialized route completions. Never change
 * those flags, chest checks, boss rewards, objectives or the rush selector. */
static int menu_ww_return231(MenuRead149 read,MenuWrite149 write,MenuPermit149 permit,
        void *p,uint64_t owner,uint32_t hub_bit) {
    MenuPair149 before,again,target;int restored=1;
    if(!hub_bit || (hub_bit&(hub_bit-1)) || !permit(p) ||
       !menu_read149(read,p,owner,&before) ||
       !menu_story_pair157(before.story,before.map[0]) ||
       memcmp(before.map[0],before.map[1],32) ||
       !menu_switch_pair232(before.switches,before.story,before.map[0]) ||
       !memchr(before.map[0],0,32) ||
       strncmp(before.map[0],"/Game/Levels/dw/dw_",sizeof("/Game/Levels/dw/dw_")-1) ||
       !(before.discovery[0]&hub_bit) || !(before.discovery[1]&hub_bit))return 0;
    /* A hub-local return already has its own authoritative stage. */
    if(!memcmp(before.map[0],"/Game/Levels/dw/dw_03/dw_03",26) &&
       before.story[0]>=200 && before.story[0]<=280)return 1;
    unsigned routes=(before.switches[0][2]>>3)&7;
    unsigned count=(routes&1)+((routes>>1)&1)+((routes>>2)&1);
    unsigned complete=before.switches[0][5]&8;
    /* All three routes without the final boss flag may be an already-corrupt
     * old development save. Do not silently bless or reset its progression. */
    if(count==3 && !complete)return 0;
    uint32_t story=complete?270:count==2?250:count==1?240:210;
    target=before;target.story[0]=target.story[1]=story;
    if(!permit(p) || !menu_read149(read,p,owner,&again) || memcmp(&before,&again,sizeof(before)))return 0;
    for(unsigned i=0;i<2;i++)
        if(!write(p,owner+(uint64_t)i*0x3d28+0x3338,&story,4))goto rollback;
    if(permit(p) && menu_read149(read,p,owner,&again) && !memcmp(&target,&again,sizeof(target)))return 1;
rollback:
    for(unsigned i=0;i<2;i++)
        if(!write(p,owner+(uint64_t)i*0x3d28+0x3338,&before.story[i],4))restored=0;
    return restored && menu_read149(read,p,owner,&again) && !memcmp(&before,&again,sizeof(before))?2:3;
}
#endif
