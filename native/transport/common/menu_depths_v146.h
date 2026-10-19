#include "menu_switch_pair_v232.h"
/* Legacy Depths profile, moved into the single owner. No save-file writes. */
#ifndef KH02_MENU_DEPTHS_V146_H
#define KH02_MENU_DEPTHS_V146_H
#include <stdint.h>
#include <string.h>
#include "menu_story_pair_v157.h"
typedef int (*MenuRead146)(void*,uint64_t,void*,uint32_t);
typedef int (*MenuWrite146)(void*,uint64_t,const void*,uint32_t);
typedef int (*MenuPermit146)(void*);
typedef struct MenuPair146 {
    uint32_t story[2];
    unsigned char switches[2][6];
    char map[2][32];
} MenuPair146;
static int menu_read146(MenuRead146 read,void *p,uint64_t owner,MenuPair146 *v) {
    memset(v,0,sizeof(*v));
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!read(p,d+0x3338,&v->story[i],4) ||
           !read(p,d+0x3538,v->switches[i],6) ||
           !read(p,d+0x384c,v->map[i],32))return 0;
    }
    return 1;
}
/* 1 committed; 0 refused without writes; 2 rolled back; 3 rollback uncertain.
 * Only the legacy four-byte story and six-byte switch spans may be written. */
static int menu_prepare146(MenuRead146 read,MenuWrite146 write,MenuPermit146 permit,
        void *p,uint64_t owner) {
    MenuPair146 before,again,target;int restored=1;
    if(!permit(p) || !menu_read146(read,p,owner,&before) ||
       !menu_story_pair157(before.story,before.map[0]) ||
       !menu_switch_pair232(before.switches,before.story,before.map[0]) ||
       memcmp(before.map[0],before.map[1],32) ||
       strncmp(before.map[0],"/Game/Levels/dw/dw_",sizeof("/Game/Levels/dw/dw_")-1) ||
       !memchr(before.map[0],0,32))return 0;
    target=before;
    for(unsigned i=0;i<2;i++) {
        target.story[i]=360;
        target.switches[i][5]&=0xbf;
    }
    if(!permit(p) || !menu_read146(read,p,owner,&again) ||
       memcmp(&before,&again,sizeof(before)))return 0;
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!write(p,d+0x3338,&target.story[i],4) ||
           !write(p,d+0x3538,target.switches[i],6))goto rollback;
    }
    if(permit(p) && menu_read146(read,p,owner,&again) &&
       !memcmp(&target,&again,sizeof(target)))return 1;
rollback:
    /* Restore both spans even if a failed write reported partial completion. */
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!write(p,d+0x3338,&before.story[i],4))restored=0;
        if(!write(p,d+0x3538,before.switches[i],6))restored=0;
    }
    return restored && menu_read146(read,p,owner,&again) &&
        !memcmp(&before,&again,sizeof(before))?2:3;
}
#endif
