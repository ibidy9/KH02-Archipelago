/* Reuse the proven single owner of stat/travel capture. No second hook owner. */
#include "kh02_grant_owner_candidate.c"
#include "kh02_controlled_owner_candidate.h"
static Kh02MixedDelivery *controlled_delivery;
static Kh02GrantCheck controlled_permit;
static void *controlled_context;
static uint32_t controlled_active(void *unused) {
    (void)unused;
    return controlled_permit && controlled_permit(controlled_context)==1 &&
        is_active(NULL) && controlled_permit(controlled_context)==1;
}
uint32_t Kh02ControlledOwnerBind(Kh02SaveRouter *router,Kh02GrantCheck permit,void *context) {
    if(controlled_delivery || !permit || !on_thread() || !life)return KH02_MEMBER_REFUSED;
    controlled_permit=permit;controlled_context=context;game.active=controlled_active;
    controlled_delivery=Kh02MixedDeliveryCreate(router,&game);
    return controlled_delivery?KH02_MEMBER_OK:KH02_MEMBER_REFUSED;
}
uint32_t Kh02ControlledOwnerExecute(uint64_t epoch,uint64_t sequence,uint32_t kind,
    const unsigned char *proposal,uint32_t size,Kh02MixedOutcome *out) {
    return Kh02MixedDeliveryExecute(controlled_delivery,epoch,sequence,kind,proposal,size,out);
}
uint32_t Kh02ControlledOwnerProbe(uint64_t *epoch,unsigned char *data,uint32_t capacity,uint32_t *size) {
    uint32_t status=Kh02MixedDeliveryProbe(controlled_delivery,epoch,data,capacity,size);
    if(status==KH02_BANK_OK) {
        Kh02GoalObservationBind(*epoch,data,*size);
        Kh02GoalObservationService();
        MickeyService168(); /* Explicit admitted client request, not stat timing. */
#if defined(KH02_GRANT_OWNER_LIVE) || defined(KH02_GRANT_OWNER_SELFTEST)
        local_service193(); /* Same authenticated command3 scope; no stat callback grant. */
#endif
    } else Kh02GoalObservationGap();
    if(status==KH02_BANK_OK && !popup_ready273(read_local,NULL,game.base,session.context)) {
        *epoch=0;*size=0;return KH02_BANK_BUSY;
    }
    return status;
}
