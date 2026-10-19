#ifndef KH02_START_PLAN_V55_H
#define KH02_START_PLAN_V55_H
#include <stdint.h>
/* Pure decision component. It never reads/writes game memory or resolves names.
 * The owner adapter must validate records and commit/read back story280 BEFORE
 * applying a returned route. Numeric FNames are supplied from this process. */
typedef struct Kh02StartPlan {
    uint64_t owner,cookie,epoch;
    uint32_t thread,phase;
} Kh02StartPlan;
typedef struct Kh02StartRequest {
    uint64_t owner,cookie,epoch,map,spawn;
    uint64_t intro_map,intro_spawn,road_map,road_spawn,depths_map,depths_spawn;
    uint32_t thread,caller,authorized,records_verified;
    uint32_t primary_story,checkpoint_story;
} Kh02StartRequest;
typedef struct Kh02StartDecision {uint64_t map,spawn;uint32_t story,redirect;} Kh02StartDecision;
/* phase:0 unarmed,1 authenticated fresh,2 observed initial intro route,
 * 3 consumed/refused. A failed commit must never re-arm this token. */
static int Kh02StartBind(Kh02StartPlan *p,uint64_t owner,uint64_t cookie,uint64_t epoch,uint32_t thread) {
    if(!p || p->phase || !owner || !cookie || epoch!=1 || !thread)return 0;
    p->owner=owner;p->cookie=cookie;p->epoch=epoch;p->thread=thread;p->phase=1;return 1;
}
static int Kh02StartDecide(Kh02StartPlan *p,const Kh02StartRequest *q,Kh02StartDecision *d) {
    if(!p || !q || !d)return 0;
    *d=(Kh02StartDecision){q->map,q->spawn,0,0};
    if(!p->phase || p->phase==3)return 0;
    if(!q->authorized || !q->records_verified || q->owner!=p->owner || q->cookie!=p->cookie ||
       q->epoch!=p->epoch || q->thread!=p->thread || q->primary_story || q->checkpoint_story) {
        p->phase=3;return 0;
    }
    if(p->phase==1 && q->caller==0x45932a && q->intro_map && q->intro_spawn &&
       q->map==q->intro_map && q->spawn==q->intro_spawn) {
        p->phase=2;return 0;
    }
    if(p->phase==2 && q->caller==0x43eca3 && q->road_map && q->road_spawn &&
       q->depths_map && q->depths_spawn && q->map==q->road_map && q->spawn==q->road_spawn) {
        p->phase=3; /* Consume before the adapter attempts any write or call. */
        *d=(Kh02StartDecision){q->depths_map,q->depths_spawn,360,1};return 1;
    }
    /* Unexpected travel is not a reason to leave a reusable pending redirect. */
    p->phase=3;return 0;
}
#endif
