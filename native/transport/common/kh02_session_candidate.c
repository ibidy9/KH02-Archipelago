#include "kh02_session_candidate.h"
#include <string.h>

_Static_assert(sizeof(Kh02SessionSample)==80,"sample ABI");
_Static_assert(sizeof(Kh02SessionCandidate)==136,"session ABI");

static uint32_t revoke(Kh02SessionCandidate *s) {
    s->context=s->world=0;
    return Kh02GateInvalidate(&s->gate);
}
static int pointer(uint64_t p) {
    return p>=0x10000 && p<=0x7fffffffffffULL && !(p&7);
}
static int admissible(const Kh02SessionCandidate *s,const Kh02SessionSample *v) {
    return v && !s->stopped && !s->uncertain && s->gate.current.cookie &&
        s->gate.current.generation && v->generation==s->gate.current.generation &&
        v->guards_verified==1 && !v->reserved && v->phase==3 && !v->title_flags &&
        !v->pending_map && !(v->travel_flags&1) && v->world_flag==0 &&
        pointer(v->context) && pointer(v->world) && pointer(v->state) &&
        pointer(v->provider) && pointer(v->attributes);
}
uint32_t Kh02SessionInit(Kh02SessionCandidate *s,uint64_t cookie) {
    if(!s || s->gate.current.cookie || !cookie)return KH02_GATE_REFUSED;
    memset(s,0,sizeof(*s));
    return Kh02GateInit(&s->gate,cookie);
}
uint32_t Kh02SessionObserve(Kh02SessionCandidate *s,const Kh02SessionSample *v) {
    if(!s)return KH02_GATE_REFUSED;
    if(!admissible(s,v)) { revoke(s);return KH02_GATE_REFUSED; }
    /* A world/context replacement revokes old observations even if stat pointers
       are reused. Require a new sample stamped with the new epoch. */
    if(s->context && (s->context!=v->context || s->world!=v->world)) {
        revoke(s);return KH02_GATE_REFUSED;
    }
    if(Kh02GateObserve(&s->gate,v->state,v->provider,v->attributes)!=KH02_GATE_OK)
        return KH02_GATE_REFUSED;
    s->context=v->context;s->world=v->world;
    return KH02_GATE_OK;
}
uint32_t Kh02SessionTravel(Kh02SessionCandidate *s,const Kh02RoutePolicy *p,
        const Kh02RouteRequest *q,Kh02RouteResult *out) {
    uint32_t decision;
    if(!s)return KH02_INVALID;
    if(!q || !out) { revoke(s);return KH02_INVALID; }
    /* Decide using the current policy BEFORE revoking delivery. Never rewrite
       a policy epoch to make it match a newer delivery generation. */
    if(q->owner_cookie!=s->gate.current.cookie ||
       q->generation!=s->gate.current.generation || !s->gate.current.generation) {
        out->map=q->map;out->spawn=q->spawn;out->fade=q->fade;
        out->rule_index=UINT32_MAX;decision=KH02_STALE;
    } else decision=Kh02DecideRoute(p,q,out);
    revoke(s); /* Unmatched and rejected policies still revoke delivery. */
    return decision;
}
uint32_t Kh02SessionExecute(Kh02SessionCandidate *s,const Kh02Lease *lease,uint64_t seq,
        const Kh02SessionSample *v,Kh02SimulatedEffect effect,void *argument) {
    uint32_t verified,result;
    Kh02Lease owned;
    if(!s || !lease || !effect || !seq || seq<=s->last_sequence || s->gate.busy)
        return KH02_GATE_REFUSED;
    if(!admissible(s,v) || s->context!=v->context || s->world!=v->world ||
       s->gate.current.state!=v->state || s->gate.current.provider!=v->provider ||
       s->gate.current.attributes!=v->attributes) {
        revoke(s);return KH02_GATE_REFUSED;
    }
    owned=*lease;
    if(Kh02GateConfirm(&s->gate,&owned)!=KH02_GATE_OK ||
       Kh02GateBegin(&s->gate,&owned)!=KH02_GATE_OK)return KH02_GATE_REFUSED;
    s->last_sequence=seq; /* Consume before callback: nested calls cannot replay. */
    verified=effect(argument);
    result=Kh02GateFinish(&s->gate,&owned,verified);
    s->gate.ready=0; /* Every effect needs a fresh admission check. */
    if(result!=KH02_GATE_OK)s->uncertain=1;
    return result;
}
uint32_t Kh02SessionStop(Kh02SessionCandidate *s) {
    if(!s)return KH02_GATE_REFUSED;
    s->stopped=1;revoke(s);
    return s->gate.busy?KH02_GATE_UNCERTAIN:KH02_GATE_OK;
}
