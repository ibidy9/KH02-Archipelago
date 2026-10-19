/* Candidate schema5 pure ownership projection. No hooks or game effects. */
#include <stdint.h>
#include <stddef.h>
static const uint64_t head[]={UINT64_C(0x7),UINT64_C(0x38),UINT64_C(0x1c0),UINT64_C(0xe00)};
static const uint64_t arms[]={UINT64_C(0xe00000),UINT64_C(0x7000000),UINT64_C(0x38000000)};
static const uint64_t back[]={UINT64_C(0x7000),UINT64_C(0x38000),UINT64_C(0x1c0000)};
static const uint64_t pattern[]={UINT64_C(0x1c0000000),UINT64_C(0xe00000000),UINT64_C(0x7000000000),UINT64_C(0x38000000000),UINT64_C(0x1c0000000000),UINT64_C(0x4c00000000000),UINT64_C(0xb000000000000)};
static int complete(uint64_t owned,const uint64_t *groups,size_t n) {
    for(size_t i=0;i<n;i++)if((owned&groups[i])==groups[i])return 1;
    return 0;
}
/* goal1=Phantom,2=Tide; requirement1=Wayfinders,2=Zodiacs,3=wardrobe.
 * starting world0=Castle,1=WW,2=Forest,3=Depths. Result bits1=Tide,2=rush.
 * Caller must provide accepted history and authenticated immutable config. */
__declspec(dllexport) uint32_t Kh02GoalOwnershipV79(const uint64_t *items,uint32_t count,
    uint32_t schema,uint32_t goal,uint32_t requirement,uint32_t start,uint32_t *out) {
    uint32_t ways=0,zodiac=0,worlds=0,ready;uint64_t wardrobe=0;
    if(!out)return 0;*out=0;
    if(schema!=5 || (goal!=1 && goal!=2) || requirement<1 || requirement>3 ||
       start>3 || count>4096 || (count && !items))return 0;
    worlds=1u<<start;
    for(uint32_t i=0;i<count;i++) {
        uint64_t id=items[i];
        if(id>=0x020220 && id<=0x020222)ways|=1u<<(id-0x020220);
        else if(id>=0x020223 && id<=0x02022e)zodiac|=1u<<(id-0x020223);
        else if(id>=0x020212 && id<=0x020215)worlds|=1u<<(id-0x020212);
        else if(id>=0x020300 && id<=0x020333 && id!=0x02032d)wardrobe|=UINT64_C(1)<<(id-0x020300);
    }
    ready=requirement==1?ways==7:requirement==2?zodiac==4095:
        complete(wardrobe,head,4)&&complete(wardrobe,arms,3)&&complete(wardrobe,back,3)&&complete(wardrobe,pattern,7);
    if(goal==2 && (worlds&8) && ready)*out|=1;
    if((worlds&2) && (goal==1?ready:zodiac==4095))*out|=2;
    return 1;
}
