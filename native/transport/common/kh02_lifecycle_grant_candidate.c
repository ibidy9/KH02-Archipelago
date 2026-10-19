#include "kh02_lifecycle_grant_candidate.h"
typedef struct GrantScope {
    Kh02Lifecycle *life;uint64_t epoch;Kh02ReceiptGrant original;
} GrantScope;
static uint32_t current(GrantScope *s) {
    return Kh02LifecycleEpoch(s->life)==s->epoch &&
        Kh02LifecycleCurrent(s->life)==s->original.owner;
}
static uint32_t check(void *context) {
    GrantScope *s=context;
    return current(s) && s->original.check(s->original.context)==1 && current(s);
}
static uint32_t read_sample(void *context,Kh02SessionSample *sample,Kh02InventorySnapshot *inventory) {
    GrantScope *s=context;
    return current(s) && s->original.read(s->original.context,sample,inventory)==1 && current(s);
}
static uint32_t convert(void *context,uint32_t battle,Kh02ReceiptItemName *name) {
    GrantScope *s=context;
    return current(s) && s->original.convert(s->original.context,battle,name)==1 && current(s);
}
static void grant(void *context,const Kh02ReceiptItemName *name,uint32_t show) {
    GrantScope *s=context;
    if(current(s))s->original.grant(s->original.context,name,show);
}
uint32_t Kh02LifecycleGrant(Kh02Lifecycle *life,uint64_t epoch,uint64_t sequence,
    const Kh02SessionSample *sample,const unsigned char *proposal,uint32_t size,Kh02ReceiptGrant *output) {
    GrantScope scope;Kh02ReceiptGrant packet;uint32_t result;
    if(!output)return KH02_MEMBER_REFUSED;
    scope.original=*output;scope.life=life;scope.epoch=epoch;
    output->entered=0;output->observed_before=output->observed_after=UINT32_MAX;
    if(!life || !epoch || !scope.original.owner || !scope.original.session ||
       !scope.original.check || !scope.original.read || !scope.original.convert ||
       !scope.original.grant || !current(&scope))return KH02_MEMBER_REFUSED;
    packet=scope.original;packet.check=check;packet.read=read_sample;
    packet.convert=convert;packet.grant=grant;packet.context=&scope;
    packet.entered=0;packet.observed_before=packet.observed_after=UINT32_MAX;
    result=Kh02ReceiptSessionExecute(packet.owner,packet.session,&packet.lease,sequence,
        sample,proposal,size,Kh02ReceiptGrantExecute,&packet);
    output->entered=packet.entered;output->observed_before=packet.observed_before;
    output->observed_after=packet.observed_after;
    return result;
}

/* Local consumable transaction with the same epoch/owner callback wrappers.
 * No AP identity, receipt proposal or AP command sequence is created. */
uint32_t Kh02LifecycleLocalGrant193(Kh02Lifecycle *life,uint64_t epoch,uint32_t battle,Kh02ReceiptGrant *output) {
    GrantScope scope;Kh02ReceiptGrant packet;uint32_t result;
    if(!output)return 0;
    scope.original=*output;scope.life=life;scope.epoch=epoch;
    output->entered=0;output->observed_before=output->observed_after=UINT32_MAX;
    if(!life || !epoch || !scope.original.owner || !scope.original.session ||
       !scope.original.check || !scope.original.read || !scope.original.convert ||
       !scope.original.grant || !current(&scope))return 0;
    packet=scope.original;packet.check=check;packet.read=read_sample;
    packet.convert=convert;packet.grant=grant;packet.context=&scope;
    packet.entered=0;packet.observed_before=packet.observed_after=UINT32_MAX;
    result=Kh02LocalConsumableGrant193(battle,&packet);
    output->entered=packet.entered;output->observed_before=packet.observed_before;
    output->observed_after=packet.observed_after;return result;
}
