#include "menu_switch_pair_v232.h"
/* Legacy Forest profile, moved into the single owner. No save-file writes. */
#ifndef KH02_MENU_FOREST_V149_H
#define KH02_MENU_FOREST_V149_H
#include <stdint.h>
#include <string.h>
#include "menu_story_pair_v157.h"
typedef int (*MenuRead149)(void*,uint64_t,void*,uint32_t);
typedef int (*MenuWrite149)(void*,uint64_t,const void*,uint32_t);
typedef int (*MenuPermit149)(void*);
typedef struct MenuPair149 {
    uint32_t story[2];
    uint32_t discovery[2];
    unsigned char switches[2][6];
    char map[2][32];
} MenuPair149;
static int menu_read149(MenuRead149 read,void *p,uint64_t owner,MenuPair149 *v) {
    memset(v,0,sizeof(*v));
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!read(p,d+0x3338,&v->story[i],4) ||
           !read(p,d+0x3848,&v->discovery[i],4) ||
           !read(p,d+0x3538,v->switches[i],6) ||
           !read(p,d+0x384c,v->map[i],32))return 0;
    }
    return 1;
}
/* 1 committed; 0 refused without writes; 2 rolled back; 3 rollback uncertain.
 * mode0: legacy restart plus explicitly selected discovery bits; mode1: only
 * story321; mode2: legacy Phantom3 story/event profile. All other bytes survive. */
static int menu_prepare214(MenuRead149 read,MenuWrite149 write,MenuPermit149 permit,
        void *p,uint64_t owner,unsigned mode,uint32_t clear_discovery) {
    MenuPair149 before,again,target;int restored=1;
    if(mode>4 || (mode && clear_discovery) || !permit(p) || !menu_read149(read,p,owner,&before) ||
       !menu_story_pair157(before.story,before.map[0]) ||
       !menu_switch_pair232(before.switches,before.story,before.map[0]) ||
       memcmp(before.map[0],before.map[1],32) ||
       strncmp(before.map[0],"/Game/Levels/dw/dw_",sizeof("/Game/Levels/dw/dw_")-1) ||
       !memchr(before.map[0],0,32))return 0;
    if(mode>=3) {
        unsigned mask=mode==3?4u:32u;
        if(!(before.switches[0][5]&mask) || !(before.switches[1][5]&mask))return 0;
        if(mode==3 && ((before.switches[0][1]&0xf0)!=0xf0 || (before.switches[1][1]&0xf0)!=0xf0))return 0;
    }
    target=before;
    for(unsigned i=0;i<2;i++) {
        if(mode==0) {
            target.story[i]=280;
            target.switches[i][3]&=0x6f;
            target.switches[i][4]&=0xe1;
            target.switches[i][5]&=0xcf;
            target.discovery[i]&=~clear_discovery;
        } else if(mode==1)target.story[i]=321;
        else if(mode==3 || mode==4) {
            /* Later authoritative handoff: Castle150 / Forest340. */
            target.story[i]=mode==3?150:340;
            target.switches[i][5]&=(unsigned char)~(mode==3?4u:32u);
        } else {
            target.story[i]=250;
            target.switches[i][2]=(unsigned char)((target.switches[i][2]&0xf0)|1);
            target.switches[i][5]&=0xf7;
        }
    }
    if(!permit(p) || !menu_read149(read,p,owner,&again) ||
       memcmp(&before,&again,sizeof(before)))return 0;
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!write(p,d+0x3338,&target.story[i],4) ||
           (mode!=1 && !write(p,d+0x3538,target.switches[i],6)) ||
           (clear_discovery && !write(p,d+0x3848,&target.discovery[i],4)))goto rollback;
    }
    if(permit(p) && menu_read149(read,p,owner,&again) &&
       !memcmp(&target,&again,sizeof(target)))return 1;
rollback:
    /* Restore both spans even if a failed write reported partial completion. */
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!write(p,d+0x3338,&before.story[i],4))restored=0;
        if(mode!=1 && !write(p,d+0x3538,before.switches[i],6))restored=0;
        if(clear_discovery && !write(p,d+0x3848,&before.discovery[i],4))restored=0;
    }
    return restored && menu_read149(read,p,owner,&again) &&
        !memcmp(&before,&again,sizeof(before))?2:3;
}
static inline int menu_prepare149(MenuRead149 read,MenuWrite149 write,MenuPermit149 permit,void *p,uint64_t owner) {
    return menu_prepare214(read,write,permit,p,owner,0,0);
}
#endif
