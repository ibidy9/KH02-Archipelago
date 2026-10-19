/* Pure legacy profile planning. No reads, writes, hooks or lifecycle ownership.
 * Caller must validate both records and transact the returned changes. */
#ifndef KH02_WORLD_PROFILES_V150_H
#define KH02_WORLD_PROFILES_V150_H
#include <stdint.h>
#include <string.h>
enum { WORLD_CASTLE150=0, WORLD_WITHIN150=1, WORLD_FOREST150=2,
       WORLD_DEPTHS150=3, CASTLE_GEARS150=4 };
typedef struct WorldProfile150 {
    uint32_t story;
    unsigned char switches[6];
} WorldProfile150;
/* Chest3 is treasure byte1 bit10, NOT a mirror completion flag.
 * Castle gear stage must additionally be admitted after player control. */
static int world_profile150(unsigned kind, const WorldProfile150 *before,
        unsigned char treasure1, int castle_control, WorldProfile150 *after) {
    if(!before || !after || kind>CASTLE_GEARS150)return 0;
    if(kind==CASTLE_GEARS150 && (!castle_control || before->story!=110))return 0;
    *after=*before;
    switch(kind) {
    case WORLD_CASTLE150:
        after->story=0;
        /* Registered flag0 is DW_TUTORIAL_ALLEND. Failed v159 retained this
         * prior answer at its story10 stall. Candidate (causality unproven)
         * replays that choice only on explicit Castle menu restart/travel;
         * individual tutorial/gear flags and normal New Game are untouched. */
        after->switches[0]&=0xfe;break;
    case WORLD_WITHIN150:
        after->story=200;after->switches[2]=0;
        after->switches[3]&=0xfe;
        if(!(treasure1&0x10))after->switches[3]&=0xfd;
        after->switches[5]&=0xf6;break;
    case WORLD_FOREST150:
        after->story=280;after->switches[3]&=0x6f;
        after->switches[4]&=0xe1;after->switches[5]&=0xcf;break;
    case WORLD_DEPTHS150:
        after->story=360;after->switches[5]&=0xbf;break;
    case CASTLE_GEARS150:
        after->switches[0]=0x7d;after->switches[1]=0x0d;
        after->switches[5]&=0xfb; /* Castle current-cycle completion; reward journal untouched. */
        break;
    }
    return 1;
}
#endif
