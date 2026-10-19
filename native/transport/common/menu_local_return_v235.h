#include "menu_switch_pair_v232.h"
#ifndef KH02_MENU_LOCAL_RETURN_V235_H
#define KH02_MENU_LOCAL_RETURN_V235_H
#include "menu_forest_v149.h"
/* Ordinary earned Castle partial-gear / Depths post-Tower return only.
 * Story-only transaction. Full-gear Castle remains unchanged pending evidence. */
static int menu_local_return235(MenuRead149 read,MenuWrite149 write,MenuPermit149 permit,
        void *p,uint64_t owner,uint32_t hub_bit,unsigned world) {
    MenuPair149 before,again,target;int restored=1;
    if(!hub_bit || (hub_bit&(hub_bit-1)) || !permit(p) ||
       !menu_read149(read,p,owner,&before) ||
       !menu_story_pair157(before.story,before.map[0]) ||
       memcmp(before.map[0],before.map[1],32) ||
       !menu_switch_pair232(before.switches,before.story,before.map[0]) ||
       !memchr(before.map[0],0,32) ||
       strncmp(before.map[0],"/Game/Levels/dw/dw_",sizeof("/Game/Levels/dw/dw_")-1) ||
       !(before.discovery[0]&hub_bit) || !(before.discovery[1]&hub_bit))return 0;
    uint32_t story;
    if(world==0) {
        if(!memcmp(before.map[0],"/Game/Levels/dw/dw_02/dw_02",26) && before.story[0]>=110 && before.story[0]<=180)return 1;
        unsigned gears=before.switches[0][1]>>4;
        unsigned count=(gears&1)+((gears>>1)&1)+((gears>>2)&1)+((gears>>3)&1);
        /* Castle gear reset now clears this cycle bit. Completion restores it.
         * Never touch inventory, journal or the retained native reward list. */
        story=count==4?((before.switches[0][5]&4)?200:150):110+10*count;
    } else if(world==3) {
        if(!memcmp(before.map[0],"/Game/Levels/dw/dw_09/dw_09",26) && before.story[0]>=410 && before.story[0]<=500)return 1;
        if(!(before.switches[0][5]&0x40) || !(before.switches[1][5]&0x40))return 0;
        story=410;
    } else return 0;
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
