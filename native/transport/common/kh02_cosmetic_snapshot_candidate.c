#include "kh02_cosmetic_snapshot_candidate.h"
#include <string.h>

static int pointer(uint64_t p) {
    return p>=0x10000 && p<=0x7fffffffffffULL-0x10000 && !(p&7);
}
static int read(Kh02BoundedRead f,void *u,uint64_t a,void *out,uint32_t n) {
    return f && n && n<=64 && a>=0x10000 && a<=0x7fffffffffffULL-n && f(u,a,out,n)==1;
}
static int block(Kh02BoundedRead f,void *u,uint64_t a,void *out,uint32_t n) {
    uint32_t offset;
    for(offset=0;offset<n;offset+=64) {
        uint32_t size=n-offset; if(size>64)size=64;
        if(!read(f,u,a+offset,(unsigned char*)out+offset,size))return 0;
    }
    return 1;
}
static int sample(Kh02BoundedRead f,void *u,uint64_t owner,Kh02CosmeticSnapshot *s) {
    unsigned char map[18];uint32_t j,i;
    s->owner=owner;
    for(j=0;j<2;j++) {
        uint64_t base=owner+(j?0x3d28:0);
        Kh02CosmeticBank *bank=j?&s->reference:&s->primary;
        if(!read(f,u,base+0x3838+0x14,map,sizeof(map)) || memcmp(map,"/Game/Levels/dw/dw",18) ||
           !block(f,u,base+0x338+0xc20,bank->objectives,sizeof(bank->objectives)) ||
           !block(f,u,base+0x338+0xcf4,bank->items,sizeof(bank->items)))return 0;
        /* All 0x310 records are the exact contiguous native lookup bank,
           not a process scan. Preserve opaque flags and unused record data;
           validate count semantics only in the traced cosmetic category. */
        for(i=0x13e;i<0x172;i++)if(bank->items[i][0]>99)return 0;
    }
    return 1;
}
uint32_t Kh02ReadCosmeticSnapshot(Kh02BoundedRead f,void *u,uint64_t world,Kh02CosmeticSnapshot *out) {
    Kh02CosmeticSnapshot first={0},second={0};uint64_t owner,again;
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(!pointer(world) || !read(f,u,world+0x158,&owner,8) || !pointer(owner) ||
       !sample(f,u,owner,&first) || !read(f,u,world+0x158,&again,8) || again!=owner ||
       !sample(f,u,owner,&second) || !read(f,u,world+0x158,&again,8) || again!=owner ||
       memcmp(&first,&second,sizeof(first)))return KH02_GATE_REFUSED;
    *out=first;return KH02_GATE_OK;
}
uint32_t Kh02CosmeticFirstAwardEligible(const Kh02CosmeticSnapshot *s) {
    /* Require the native completable state (low nibble 1), not merely an
       absent serialized completion bit. Require pristine zero-stock records
       in both banks for this single-item experiment. */
    return s && pointer(s->owner) &&
        (s->primary.objectives[KH02_COSMETIC_TEST_OBJECTIVE]&15)==1 &&
        (s->reference.objectives[KH02_COSMETIC_TEST_OBJECTIVE]&15)==1 &&
        s->primary.items[KH02_COSMETIC_RECORD][0]==0 &&
        s->primary.items[KH02_COSMETIC_RECORD][1]==0 &&
        s->reference.items[KH02_COSMETIC_RECORD][0]==0 &&
        s->reference.items[KH02_COSMETIC_RECORD][1]==0;
}
uint32_t Kh02CosmeticFirstAwardVerified(const Kh02CosmeticSnapshot *before,const Kh02CosmeticSnapshot *after) {
    uint32_t i;
    if(!after || !Kh02CosmeticFirstAwardEligible(before) || before->owner!=after->owner ||
       memcmp(before->primary.objectives,after->primary.objectives,sizeof(before->primary.objectives)) ||
       memcmp(&before->reference,&after->reference,sizeof(before->reference)))return 0;
    for(i=0;i<KH02_NATIVE_ITEM_RECORDS;i++) {
        if(i==KH02_COSMETIC_RECORD) {
            if(after->primary.items[i][0]!=1 || after->primary.items[i][1]!=3)return 0;
        } else if(memcmp(before->primary.items[i],after->primary.items[i],sizeof(before->primary.items[i])))return 0;
    }
    return 1;
}
