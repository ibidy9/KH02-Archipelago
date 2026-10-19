#include "kh02_difficulty_observation.h"
#include <string.h>
static int ptr(uint64_t p,uint64_t extent) {
    return p>=0x10000 && !(p&7) && p<=0x0000800000000000ULL-extent;
}
uint64_t Kh02DifficultyObserve(Kh02BoundedRead read,void *opaque,uint64_t base,uint64_t context) {
    uint64_t table,again,methods[2];uint32_t a,b,x,y;
    if(!read || !ptr(base,0x4f5f000) || !ptr(context,0x4264))return 2;
    if(!read(opaque,context,&table,8))return 3;
    if(table!=base+0x36df228)return 4;
    if(!read(opaque,table+0x390,methods,16))return 3;
    if(methods[0]!=base+0x447b20 || methods[1]!=base+0x453d60)return 4;
    if(!read(opaque,context+0x538,&a,4) || !read(opaque,context+0x4260,&b,4) ||
       !read(opaque,context+0x538,&x,4) || !read(opaque,context+0x4260,&y,4) ||
       !read(opaque,context,&again,8))return 3;
    if(table!=again || a!=x || b!=y)return 5;
    if(a>3 || b>3)return 6;
    return 1|((uint64_t)a<<8)|((uint64_t)b<<16);
}
uint64_t Kh02DifficultyObserveRoot(Kh02BoundedRead read,void *opaque,uint64_t base) {
    uint64_t root,link,context,r2,l2,c2,result;
    /* Exact pointer path from54AFD0. Instead of invoking its class initializer,
       require the established game-instance vtable/methods in Observe. */
    if(!read || !ptr(base,0x4f5f000))return 2;
    if(!read(opaque,base+0x480b5d0,&root,8))return 3;
    if(!ptr(root,0x238))return 2;
    if(!read(opaque,root+0x230,&link,8))return 3;
    if(!ptr(link,0x58))return 2;
    if(!read(opaque,link+0x50,&context,8))return 3;
    result=Kh02DifficultyObserve(read,opaque,base,context);
    if(!read(opaque,base+0x480b5d0,&r2,8) || !read(opaque,root+0x230,&l2,8) ||
       !read(opaque,link+0x50,&c2,8))return 3;
    return root==r2 && link==l2 && context==c2?result:5;
}
