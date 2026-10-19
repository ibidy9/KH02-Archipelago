#include "kh02_generation_candidate.h"
#include <string.h>
_Static_assert(sizeof(Kh02Lease)==48,"lease ABI");
_Static_assert(sizeof(Kh02GenerationGate)==104,"gate ABI");

static int valid(const Kh02GenerationGate *g) {
    return g && g->current.cookie && g->current.generation;
}
static int matches(const Kh02GenerationGate *g,const Kh02Lease *lease) {
    return valid(g) && lease && !memcmp(&g->current,lease,sizeof(*lease));
}
static int pointer(uint64_t p) {
    return p>=0x10000 && p<=0x7fffffffffffULL && !(p&7);
}
uint32_t Kh02GateInit(Kh02GenerationGate *g,uint64_t cookie) {
    if(!g || !cookie || g->current.cookie)return KH02_GATE_REFUSED;
    memset(g,0,sizeof(*g));g->current.cookie=cookie;g->current.generation=1;
    return KH02_GATE_OK;
}
uint32_t Kh02GateInvalidate(Kh02GenerationGate *g) {
    if(!valid(g))return KH02_GATE_REFUSED;
    g->ready=0;g->current.observation=0;
    g->current.state=g->current.provider=g->current.attributes=0;
    /* Saturation closes permanently. Never wrap to a previously issued lease. */
    if(g->current.generation==UINT64_MAX) {
        g->current.generation=0;return KH02_GATE_REFUSED;
    }
    g->current.generation++;
    return KH02_GATE_OK;
}
uint32_t Kh02GateObserve(Kh02GenerationGate *g,uint64_t state,uint64_t provider,uint64_t attributes) {
    if(!valid(g))return KH02_GATE_REFUSED;
    if(!pointer(state)||!pointer(provider)||!pointer(attributes)) {
        Kh02GateInvalidate(g);return KH02_GATE_REFUSED;
    }
    if(g->current.state==state && g->current.provider==provider && g->current.attributes==attributes)
        return KH02_GATE_OK; /* Same observation cannot reactivate anything. */
    if(Kh02GateInvalidate(g)!=KH02_GATE_OK)return KH02_GATE_REFUSED;
    g->current.state=state;g->current.provider=provider;g->current.attributes=attributes;
    g->current.observation=1;
    return KH02_GATE_OK;
}
uint32_t Kh02GateConfirm(Kh02GenerationGate *g,const Kh02Lease *lease) {
    if(!matches(g,lease)||g->busy||!g->current.observation)return KH02_GATE_REFUSED;
    g->ready=1;return KH02_GATE_OK;
}
uint32_t Kh02GateBegin(Kh02GenerationGate *g,const Kh02Lease *lease) {
    if(!matches(g,lease)||!g->ready||g->busy)return KH02_GATE_REFUSED;
    g->in_flight=*lease;g->busy=1;return KH02_GATE_OK;
}
uint32_t Kh02GateFinish(Kh02GenerationGate *g,const Kh02Lease *lease,uint32_t verified) {
    if(!g||!g->busy||!lease||memcmp(&g->in_flight,lease,sizeof(*lease)))return KH02_GATE_REFUSED;
    g->busy=0;
    memset(&g->in_flight,0,sizeof(g->in_flight));
    if(!matches(g,lease)||!g->ready||verified!=1) {
        Kh02GateInvalidate(g);return KH02_GATE_UNCERTAIN;
    }
    return KH02_GATE_OK;
}
