/* Additive composition: no change to either proven game award implementation. */
#include "kh02_history_router_candidate.c"
#include "kh02_mixed_delivery_candidate.h"
typedef struct MixedAttempt {
    struct MixedAttempt *next;
    Kh02MixedOutcome result;
    Kh02CosmeticAttempt *wardrobe;
} MixedAttempt;
struct Kh02MixedDelivery {
    Kh02SaveRouter *router;
    const Kh02GameGrantContext *source;
    DWORD thread;
    uint64_t served;
    uint32_t closed,busy;
    MixedAttempt *attempts;
};
typedef struct MixedBorrow {
    Kh02MixedDelivery *delivery;
    Kh02ReceiptOwner *owner;
    Kh02SavePipeline *pipeline;
    uint64_t epoch;
    Kh02GameGrantContext original;
} MixedBorrow;
static uint32_t mixed_current(MixedBorrow *b) {
    Kh02MixedDelivery *d=b->delivery;Kh02SaveRouter *p=d->router;
    const Kh02GameGrantContext *g=d->source,*s=&b->original;
    return GetCurrentThreadId()==d->thread && !d->closed && !p->closed &&
        g->base==s->base && g->cache==s->cache && g->session==s->session &&
        g->read==s->read && g->read_context==s->read_context &&
        g->active==s->active && g->active_context==s->active_context &&
        !p->life->closed && b->pipeline && !b->pipeline->stopped &&
        b->pipeline->config.owner==b->owner && b->owner &&
        Kh02LifecycleEpoch(p->life)==b->epoch && Kh02LifecycleCurrent(p->life)==b->owner;
}
static uint32_t mixed_active(void *context) {
    MixedBorrow *b=context;Kh02GameGrantContext *g=&b->original;
    return mixed_current(b) && g->active(g->active_context)==1 && mixed_current(b);
}
MEMBER_API Kh02MixedDelivery *Kh02MixedDeliveryCreate(Kh02SaveRouter *p,const Kh02GameGrantContext *game) {
    Kh02MixedDelivery *d;
    if(!p || GetCurrentThreadId()!=p->thread || p->closed || !game || !game->session ||
       !game->read || !game->active || game->base!=p->config.base ||
       game->session->gate.current.cookie!=p->life->branch->scope.cookie)return NULL;
    d=VirtualAlloc(NULL,sizeof(*d),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!d)return NULL;
    d->router=p;d->source=game;d->thread=GetCurrentThreadId();return d;
}
MEMBER_API uint32_t Kh02MixedDeliveryExecute(Kh02MixedDelivery *d,uint64_t epoch,uint64_t sequence,
    uint32_t kind,const unsigned char *proposal,uint32_t size,Kh02MixedOutcome *out) {
    Kh02CallbackPin pin={0};MixedBorrow borrow;Kh02GameGrantContext guarded;
    Kh02ReceiptGrant packet;Kh02SessionSample sample;MixedAttempt *attempt=NULL;
    unsigned char *owned=NULL;uint32_t status=KH02_MEMBER_REFUSED;
    if(!out)return status;
    *out=(Kh02MixedOutcome){epoch,sequence,status,kind,0,UINT32_MAX,UINT32_MAX,0};
    if(!d || GetCurrentThreadId()!=d->thread || d->closed || d->busy ||
       !sequence || sequence!=d->served+1 || sequence>4096)return status;
    d->busy=1;d->served=sequence; /* Consumed before allocations/readers/effects. */
    if((kind!=KH02_MIXED_CONSUMABLE && kind!=KH02_MIXED_WARDROBE) ||
       !proposal || size<56 || size>KH02_RECEIPT_MAX_BYTES)goto done;
    attempt=VirtualAlloc(NULL,sizeof(*attempt),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!attempt)goto done;
    attempt->result=*out;attempt->next=d->attempts;d->attempts=attempt;
    if(kind==KH02_MIXED_WARDROBE) {
        attempt->wardrobe=VirtualAlloc(NULL,sizeof(*attempt->wardrobe),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        if(!attempt->wardrobe)goto done;
    }
    owned=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!owned)goto done;
    memcpy(owned,proposal,size);
    if(!Kh02CallbackEnter(&d->router->domain,&pin))goto done;
    borrow=(MixedBorrow){d,Kh02LifecycleCurrent(d->router->life),pin.selected,epoch,*d->source};
    guarded=borrow.original;guarded.active=mixed_active;guarded.active_context=&borrow;
    if(!guarded.session || !guarded.read || !guarded.active || guarded.base!=d->router->config.base ||
       guarded.session->gate.current.cookie!=d->router->life->branch->scope.cookie)goto done;
    if(!mixed_active(&borrow))goto done;
    if(kind==KH02_MIXED_CONSUMABLE) {
        if(Kh02GameGrantPrepare(&guarded,borrow.owner,1,&packet,&sample)!=KH02_MEMBER_OK)goto done;
        status=Kh02ReceiptSessionExecute(borrow.owner,guarded.session,&packet.lease,sequence,
            &sample,owned,size,Kh02ReceiptGrantExecute,&packet);
        out->entered=packet.entered;out->before=packet.observed_before;out->after=packet.observed_after;
    } else {
        status=Kh02WardrobeCatalogReceiptOnce(borrow.owner,&guarded,sequence,owned,size,attempt->wardrobe);
        out->entered=attempt->wardrobe->entered;
    }
    if(status==KH02_MEMBER_OK && !mixed_active(&borrow))status=KH02_MEMBER_REFUSED;
done:
    out->status=status;if(attempt)attempt->result=*out;
    /* Unknown completion closes the graph while the selected owner is pinned. */
    if(status!=KH02_MEMBER_OK){d->closed=1;Kh02SaveRouterStop(d->router);}
    if(owned)VirtualFree(owned,0,MEM_RELEASE);
    Kh02CallbackLeave(&pin);Kh02SaveRouterCollect(d->router);d->busy=0;
    return status;
}
