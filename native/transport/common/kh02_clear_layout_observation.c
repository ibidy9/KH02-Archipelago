#include "kh02_clear_layout_observation.h"
uint64_t Kh02ClearLayoutObserve(Kh02BoundedRead read,void *context,uintptr_t state) {
    unsigned char collectibles[0x34],summary[3];uint32_t setting,count=0,i;
    if(!read || !state || state>UINTPTR_MAX-0x3209 ||
       !read(context,state+0x1f4,&setting,4) ||
       !read(context,state+0x27f8,collectibles,sizeof(collectibles)) ||
       !read(context,state+0x3206,summary,sizeof(summary)))return 0;
    for(i=0;i<sizeof(collectibles);i++)count+=collectibles[i]!=0;
    return (1ULL<<63)|((uint64_t)(setting&0x7fffffff)<<32)|((uint64_t)count<<24)|
        ((uint64_t)summary[2]<<16)|((uint64_t)summary[1]<<8)|summary[0];
}
