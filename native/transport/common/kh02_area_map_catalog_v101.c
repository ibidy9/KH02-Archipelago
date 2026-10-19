#include "kh02_area_map_v101.h"
#include <string.h>
uint32_t Kh02AreaMapResolve(uint64_t item,Kh02AreaMapDescriptor *out) {
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    /* Four candidate map codes, independently verified by native key lookup before award. */
    if(item<0x02022fULL || item>0x020232ULL)return 0;
    out->item=item;out->code=0xc00u+(uint32_t)(item-0x02022fULL);
    out->record=0x30cu+(uint32_t)(item-0x02022fULL);return 1;
}
uint32_t Kh02AreaMapFirstEligible(const Kh02CosmeticSnapshot *s,uint64_t item) {
    Kh02AreaMapDescriptor d;
    if(!s || !Kh02AreaMapResolve(item,&d) || s->owner<0x10000 ||
       s->owner>0x7fffffffffffULL-0x10000 || (s->owner&7))return 0;
    /* Objective completion is independent from AP cosmetic ownership.
       This first-ownership adapter still refuses already-owned/dirty records. */
    return !s->primary.items[d.record][0] && !s->primary.items[d.record][1] &&
           !s->reference.items[d.record][0] && !s->reference.items[d.record][1];
}
uint32_t Kh02AreaMapFirstVerified(const Kh02CosmeticSnapshot *before,const Kh02CosmeticSnapshot *after,uint64_t item) {
    Kh02AreaMapDescriptor d;uint32_t i;
    if(!after || !Kh02AreaMapResolve(item,&d) || !Kh02AreaMapFirstEligible(before,item) ||
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
