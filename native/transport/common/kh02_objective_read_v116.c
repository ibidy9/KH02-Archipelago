#include "kh02_objective_read_v116.h"
#include <string.h>

static int pointer(uint64_t p) {
    return p>=0x10000 && p<=0x7ffffffe0000ULL && !(p&7);
}
static int read(Kh02BoundedRead f,void *u,uint64_t address,void *out,uint32_t n) {
    return f && n && n<=64 && f(u,address,out,n)==1;
}
static int identity(Kh02BoundedRead f,void *u,uint64_t base,uint64_t world,uint64_t owner) {
    uint64_t observed=0,vt=0;unsigned char map[18];
    return read(f,u,world+0x158,&observed,8) && observed==owner &&
        read(f,u,owner,&vt,8) && vt==base+0x36df228 &&
        read(f,u,owner+0x384c,map,18) && !memcmp(map,"/Game/Levels/dw/dw",18);
}
static int states(Kh02BoundedRead f,void *u,uint64_t owner,uint32_t *out) {
    for(uint32_t i=0;i<51;i+=16) {
        uint32_t count=51-i;if(count>16)count=16;
        if(!read(f,u,owner+0x338+0xc20+i*4,out+i,count*4))return 0;
    }
    for(uint32_t i=0;i<51;i++)if((out[i]&15)>2)return 0;
    return 1;
}
uint32_t Kh02ObjectiveReadV116(Kh02BoundedRead f,void *u,uint64_t base,
        uint64_t world,uint64_t owner,Kh02ObjectiveStatesV116 *out) {
    Kh02ObjectiveStatesV116 first={0},second={0};
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(!pointer(base) || !pointer(world) || !pointer(owner) ||
       base>0x7ffff0000000ULL)return KH02_GATE_REFUSED;
    first.owner=second.owner=owner;
    if(!identity(f,u,base,world,owner) || !states(f,u,owner,first.states) ||
       !identity(f,u,base,world,owner) || !states(f,u,owner,second.states) ||
       !identity(f,u,base,world,owner) || memcmp(&first,&second,sizeof(first)))
        return KH02_GATE_REFUSED;
    *out=first;return KH02_GATE_OK;
}
