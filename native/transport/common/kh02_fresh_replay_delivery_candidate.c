#include "kh02_fresh_replay_mixed_candidate.c"
#include "kh02_controlled_owner_candidate.h"
/* Exact receipt query and readiness check under the same owner pin as awards.
   No item effect. BUSY is a not-ready result; unknown save ownership stays closed. */
MEMBER_API uint32_t Kh02MixedDeliveryProbe(Kh02MixedDelivery *d,uint64_t *epoch,
    unsigned char *data,uint32_t capacity,uint32_t *size) {
    Kh02CallbackPin pin={0};MixedBorrow borrow;Kh02GameGrantContext guarded;
    Kh02ReceiptGrant packet;Kh02SessionSample sample;Kh02MembershipTicket ticket={0};
    Kh02ReceiptLease lease={0};uint32_t status=KH02_BANK_BUSY;
    if(!epoch || !data || !size)return KH02_BANK_REFUSED;*epoch=0;*size=0;
    if(!d || GetCurrentThreadId()!=d->thread || d->closed || d->busy)return status;
    d->busy=1;
    if(!Kh02CallbackEnter(&d->router->domain,&pin))goto done;
    borrow=(MixedBorrow){d,Kh02LifecycleCurrent(d->router->life),pin.selected,
        Kh02LifecycleEpoch(d->router->life),*d->source};
    guarded=borrow.original;guarded.active=mixed_active;guarded.active_context=&borrow;
    if(!borrow.original.active || !borrow.original.read || !guarded.session || !mixed_active(&borrow) ||
       Kh02GameGrantPrepare(&guarded,borrow.owner,1,&packet,&sample)!=KH02_MEMBER_OK)goto done;
    if(Kh02ReceiptOwnerStageBegin(borrow.owner,&ticket)!=KH02_MEMBER_OK ||
       Kh02ReceiptOwnerStageFinish(borrow.owner,&ticket,1,&lease)!=KH02_MEMBER_OK)goto done;
    status=Kh02ReceiptOwnerRead(borrow.owner,&lease,data,capacity,size);
    if(Kh02ReceiptOwnerRelease(borrow.owner,&lease)!=KH02_BANK_OK || !mixed_active(&borrow))status=KH02_BANK_REFUSED;
    if(status==KH02_BANK_OK)*epoch=borrow.epoch;
done:
    Kh02CallbackLeave(&pin);Kh02SaveRouterCollect(d->router);d->busy=0;return status;
}
