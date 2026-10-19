#include "kh02_wardrobe_catalog_candidate.h"
#include <string.h>
uint32_t Kh02WardrobeResolve(uint64_t item,Kh02WardrobeDescriptor *out) {
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    /* Audited catalogue: every native code600..633 except default pattern62d.
       AP identities remain draft until registered in the generated world. */
    if(item<0x020300ULL || item>0x020333ULL || item==0x02032dULL)return 0;
    out->item=item;out->code=0x600u+(uint32_t)(item-0x020300ULL);
    out->record=0x13eu+(uint32_t)(item-0x020300ULL);return 1;
}
uint32_t Kh02WardrobeFirstEligible(const Kh02CosmeticSnapshot *s,uint64_t item) {
    Kh02WardrobeDescriptor d;
    if(!s || !Kh02WardrobeResolve(item,&d) || s->owner<0x10000 ||
       s->owner>0x7fffffffffffULL-0x10000 || (s->owner&7))return 0;
    /* Objective completion is independent from AP cosmetic ownership.
       This first-ownership adapter still refuses already-owned/dirty records. */
    return !s->primary.items[d.record][0] && !s->primary.items[d.record][1] &&
           !s->reference.items[d.record][0] && !s->reference.items[d.record][1];
}
uint32_t Kh02WardrobeFirstVerified(const Kh02CosmeticSnapshot *before,const Kh02CosmeticSnapshot *after,uint64_t item) {
    Kh02WardrobeDescriptor d;uint32_t i;
    if(!after || !Kh02WardrobeResolve(item,&d) || !Kh02WardrobeFirstEligible(before,item) ||
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
