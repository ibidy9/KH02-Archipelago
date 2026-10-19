#ifndef KH02_MENU_SWITCH_PAIR_V232_H
#define KH02_MENU_SWITCH_PAIR_V232_H
#include <stdint.h>
#include <string.h>
/* Permit only a completed WW route's checkpoint IN lag. This is admission,
 * never normalization: callers retain both original spans independently. */
static int menu_switch_pair232(const unsigned char flags[2][6],
        const uint32_t story[2],const char map[32]) {
    /* Departure-only compatibility. Independent spans remain unchanged. */
    if(!memcmp(map,"/Game/Levels/dw/dw_09/dw_09",26) && story[0]==410 && story[1]==400) {
        return !memcmp(flags[0],flags[1],5) && (flags[0][5]^flags[1][5])==0x40 && (flags[0][5]&0x40);
    }
    if(!memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26) && story[0]!=story[1] && story[0]>=120 && story[0]<=150) {
        unsigned a=flags[0][1]>>4,b=flags[1][1]>>4;
        unsigned na=(a&1)+((a>>1)&1)+((a>>2)&1)+((a>>3)&1);
        unsigned nb=(b&1)+((b>>1)&1)+((b>>2)&1)+((b>>3)&1);
        if(story[0]!=110+10*na || !(story[1]==110+10*nb || (story[1]==70 && !nb)) ||
           na<=nb || (b&~a) || flags[0][0]!=flags[1][0] ||
           (flags[0][1]&15)!=(flags[1][1]&15) || flags[0][2]!=flags[1][2] ||
           flags[0][4]!=flags[1][4] || flags[0][5]!=flags[1][5])return 0;
        unsigned delta=flags[0][3]^flags[1][3];
        return !delta || (delta==4 && (flags[0][3]&4));
    }
    if(!memcmp(flags[0],flags[1],6))return 1;
    if(memcmp(map,"/Game/Levels/dw/dw_03/dw_03",26) || story[0]!=story[1])return 0;
    for(unsigned i=0;i<6;i++)if(i!=2 && flags[0][i]!=flags[1][i])return 0;
    unsigned difference=flags[0][2]^flags[1][2];
    if(!difference || (difference&(difference-1)) || (difference&~7u) ||
       (flags[0][2]&7) || (flags[1][2]&7)!=difference ||
       !(flags[0][2]&(difference<<3)))return 0;
    unsigned routes=(flags[0][2]>>3)&7;
    unsigned count=(routes&1)+((routes>>1)&1)+((routes>>2)&1);
    return (count==1 && story[0]==240 && !(flags[0][5]&8)) ||
           (count==2 && story[0]==250 && !(flags[0][5]&8)) ||
           (count==3 && (story[0]==270 || story[0]==280) && (flags[0][5]&8));
}
#endif
