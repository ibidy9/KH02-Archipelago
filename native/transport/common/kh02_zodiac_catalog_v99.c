#include "kh02_zodiac_catalog_v99.h"
#include <string.h>
uint32_t Kh02ZodiacResolve(uint64_t item,Kh02ZodiacDescriptor *out) {
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    /* v64/v66 live identity: AP223..22e -> native900..90b -> record23a..245. */
    if(item<0x020223ULL || item>0x02022eULL)return 0;
    out->item=item;out->code=0x900u+(uint32_t)(item-0x020223ULL);
    out->record=0x23au+(uint32_t)(item-0x020223ULL);return 1;
}
uint32_t Kh02ZodiacFirstEligible(const Kh02CosmeticSnapshot *s,uint64_t item) {
    Kh02ZodiacDescriptor d;
    if(!s || !Kh02ZodiacResolve(item,&d) || s->owner<0x10000 ||
       s->owner>0x7fffffffffffULL-0x10000 || (s->owner&7))return 0;
    /* Objective completion is independent from an individual Zodiac receipt.
       This first-ownership adapter still refuses already-owned/dirty records. */
    return !s->primary.items[d.record][0] && !s->primary.items[d.record][1] &&
           !s->reference.items[d.record][0] && !s->reference.items[d.record][1];
}
uint32_t Kh02ZodiacFirstVerified(const Kh02CosmeticSnapshot *before,const Kh02CosmeticSnapshot *after,uint64_t item) {
    Kh02ZodiacDescriptor d;uint32_t i;
    if(!after || !Kh02ZodiacResolve(item,&d) || !Kh02ZodiacFirstEligible(before,item) ||
       before->owner!=after->owner ||
       memcmp(before->primary.objectives,after->primary.objectives,sizeof(before->primary.objectives)) ||
       memcmp(&before->reference,&after->reference,sizeof(before->reference)))return 0;
    for(i=0;i<KH02_NATIVE_ITEM_RECORDS;i++) {
        if(i==d.record) {
            if(after->primary.items[i][0]!=1 || after->primary.items[i][1]!=3)return 0;
        } else if(memcmp(before->primary.items[i],after->primary.items[i],sizeof(before->primary.items[i])))return 0;
    }
    return 1;
}
