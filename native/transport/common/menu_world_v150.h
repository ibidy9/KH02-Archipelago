#include "menu_switch_pair_v232.h"
/* Legacy Depths profile, moved into the single owner. No save-file writes. */
#ifndef KH02_MENU_WORLD_V150_H
#define KH02_MENU_WORLD_V150_H
#include <stdint.h>
#include <string.h>
#include "world_profiles_v150.h"
#include "menu_story_pair_v157.h"
typedef int (*MenuRead150)(void*,uint64_t,void*,uint32_t);
typedef int (*MenuWrite150)(void*,uint64_t,const void*,uint32_t);
typedef int (*MenuPermit150)(void*);
typedef struct MenuPair150 {
    uint32_t story[2];
    unsigned char switches[2][6];
    char map[2][32];
    unsigned char treasure1[2];
} MenuPair150;
static unsigned menu_reason157,menu_before_valid157;
static MenuPair150 menu_before157;
static int menu_read150(MenuRead150 read,void *p,uint64_t owner,MenuPair150 *v) {
    memset(v,0,sizeof(*v));
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!read(p,d+0x3338,&v->story[i],4) ||
           !read(p,d+0x3538,v->switches[i],6) ||
           !read(p,d+0x384c,v->map[i],32) ||
           !read(p,d+0x3839,&v->treasure1[i],1))return 0;
    }
    return 1;
}
/* 1 committed; 0 refused without writes; 2 rolled back; 3 rollback uncertain.
 * Only the legacy four-byte story and six-byte switch spans may be written. */
static int menu_prepare150(MenuRead150 read,MenuWrite150 write,MenuPermit150 permit,
        void *p,uint64_t owner,unsigned kind,int castle_control) {
    MenuPair150 before,again,target;int restored=1,castle_lag=0;
    menu_reason157=0;menu_before_valid157=0;
    if(!permit(p)){menu_reason157=1;return 0;}
    if(!menu_read150(read,p,owner,&before)){menu_reason157=2;return 0;}
    menu_before157=before;menu_before_valid157=1;
    if(!menu_story_pair157(before.story,before.map[0])){menu_reason157=3;return 0;}
    if(before.treasure1[0]!=before.treasure1[1]){menu_reason157=4;return 0;}
    /* v157 live Castle first-control snapshot: only byte1 bit04 lags.
     * This exception belongs solely to the authenticated gear reset, whose
     * existing profile replaces both first-two-byte spans. Keep both originals
     * independent for rollback; do not infer an individual gear from this bit. */
    /* v159: retain matching prior manual-gear bits (upper nibble) in
     * admission. The reset already owns these bits and clears them in both
     * copies. Only bit04 may differ, exactly as in the live v158 pair. */
    int castle_switch_lag158=kind==CASTLE_GEARS150 && castle_control &&
        before.story[0]==110 && before.story[1]==70 &&
        !memcmp(before.map[0],"/Game/Levels/dw/dw_02/dw_02",26) &&
        before.switches[0][0]==0x7d && before.switches[1][0]==0x7d &&
        (before.switches[0][1]&0x0f)==0x05 && (before.switches[1][1]&0x0f)==0x01 &&
        (before.switches[0][1]&0xf0)==(before.switches[1][1]&0xf0) &&
        !memcmp(before.switches[0]+2,before.switches[1]+2,4);
    if(!menu_switch_pair232(before.switches,before.story,before.map[0]) && !castle_switch_lag158){menu_reason157=5;return 0;}
    if(memcmp(before.map[0],before.map[1],32) ||
       strncmp(before.map[0],"/Game/Levels/dw/dw_",sizeof("/Game/Levels/dw/dw_")-1) ||
       !memchr(before.map[0],0,32)){menu_reason157=6;return 0;}
    castle_lag=kind==CASTLE_GEARS150 && before.story[0]==110 && before.story[1]==70;
    target=before;
    for(unsigned i=0;i<2;i++) {
        WorldProfile150 in={0},out;
        in.story=castle_lag?110:before.story[i];memcpy(in.switches,before.switches[i],6);
        if(!world_profile150(kind,&in,before.treasure1[i],castle_control,&out)){menu_reason157=7;return 0;}
        target.story[i]=castle_lag?before.story[i]:out.story;memcpy(target.switches[i],out.switches,6);
    }
    if(!permit(p)){menu_reason157=8;return 0;}
    if(!menu_read150(read,p,owner,&again) || memcmp(&before,&again,sizeof(before))){menu_reason157=9;return 0;}
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!write(p,d+0x3338,&target.story[i],4) ||
           !write(p,d+0x3538,target.switches[i],6))goto rollback;
    }
    if(permit(p) && menu_read150(read,p,owner,&again) &&
       !memcmp(&target,&again,sizeof(target)))return 1;
rollback:
    /* Restore both spans even if a failed write reported partial completion. */
    for(unsigned i=0;i<2;i++) {
        uint64_t d=owner+(uint64_t)i*0x3d28;
        if(!write(p,d+0x3338,&before.story[i],4))restored=0;
        if(!write(p,d+0x3538,before.switches[i],6))restored=0;
    }
    return restored && menu_read150(read,p,owner,&again) &&
        !memcmp(&before,&again,sizeof(before))?2:3;
}
#endif
