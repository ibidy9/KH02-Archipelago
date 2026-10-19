#include "kh02_inventory_candidate.h"
#include <string.h>
static int read(Kh02BoundedRead f,void *u,uint64_t a,void *p,uint32_t n) {
    return f && a>=0x10000 && n && n<=64 && a<=0x7fffffffffffULL-n+1 && f(u,a,p,n)==1;
}
uint32_t Kh02ReadInventory(Kh02BoundedRead f,void *u,uint64_t world,Kh02InventorySnapshot *out) {
    Kh02InventorySnapshot s={0};unsigned char block[56],map[18];uint64_t owner,again,anchor;
    uint32_t i,j;
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(world<0x10000 || world>0x7fffffffffffULL-0x10000 || (world&7) ||
       !read(f,u,world+0x158,&owner,8) || owner<0x10000 || owner>0x7fffffffffffULL-0x10000 || (owner&7))return KH02_GATE_REFUSED;
    s.owner=owner;s.primary=owner+0x3838;s.reference=s.primary+0x3d28;
    for(j=0;j<2;j++) {
        anchor=j?s.reference:s.primary;
        if(!read(f,u,anchor+0x14,map,18) || memcmp(map,"/Game/Levels/dw/dw_",18) ||
           !read(f,u,anchor-0x280c,block,56))return KH02_GATE_REFUSED;
        for(i=0;i<7;i++) {
            uint32_t count;memcpy(&count,block+i*8,4);
            if(count>99)return KH02_GATE_REFUSED;
            if(j)s.reference_counts[i]=count;else s.counts[i]=count;
        }
    }
    if(!read(f,u,world+0x158,&again,8) || owner!=again)return KH02_GATE_REFUSED;
    *out=s;return KH02_GATE_OK;
}
