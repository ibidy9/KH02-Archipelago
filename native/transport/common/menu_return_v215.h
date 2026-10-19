#include "menu_switch_pair_v232.h"
#ifndef KH02_MENU_RETURN_V215_H
#define KH02_MENU_RETURN_V215_H
#include "menu_forest_v149.h"
/* Ordinary Forest save travel only. Native discovery is serialized in the
 * existing save. Reconstruct the latest earned save milestone, never an
 * encounter-in-progress state or a new persistent side ledger. */
static int menu_return215(MenuRead149 read,MenuWrite149 write,MenuPermit149 permit,
        void *p,uint64_t owner,unsigned selected,const uint32_t bits[3]) {
    MenuPair149 before,again,target;int restored=1;
    if(selected>2 || !bits || !permit(p))return 0;
    for(unsigned i=0;i<3;i++)
        if(!bits[i] || (bits[i]&(bits[i]-1)))return 0;
    if((bits[0]&bits[1]) || (bits[0]&bits[2]) || (bits[1]&bits[2]))return 0;
    if(!menu_read149(read,p,owner,&before) ||
       !menu_story_pair157(before.story,before.map[0]) ||
       memcmp(before.map[0],before.map[1],32) ||
       !menu_switch_pair232(before.switches,before.story,before.map[0]) ||
       !memchr(before.map[0],0,32) ||
       strncmp(before.map[0],"/Game/Levels/dw/dw_",sizeof("/Game/Levels/dw/dw_")-1))return 0;
    /* A genuine Forest story is already authoritative. Never regress it or
     * overwrite an active encounter just to normalize a checkpoint. */
    if(!memcmp(before.map[0],"/Game/Levels/dw/dw_08/dw_08",26) &&
       before.story[0]>=280 && before.story[0]<=350)return 1;
    uint32_t relevant=bits[0]|bits[1]|bits[2];
    if((before.discovery[0]&relevant)!=(before.discovery[1]&relevant) ||
       !(before.discovery[0]&bits[selected]))return 0;
    /* Existing Forest Darkside completion bit, also cleared by proven restart.
     * A completed boss must not regress merely because Path's End was not yet
     * visited before leaving the world. */
    uint32_t story=((before.discovery[0]&bits[2]) || (before.switches[0][5]&0x20))?350:
                   (before.discovery[0]&bits[1])?323:300;
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
