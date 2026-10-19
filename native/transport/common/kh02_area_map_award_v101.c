#include "kh02_area_map_v101.h"
#include <string.h>
#ifndef KH02_COSMETIC_GAME_CALLS
#define KH02_COSMETIC_GAME_CALLS 0
#endif
uint32_t Kh02AreaMapAwardCallsEnabled(void){return KH02_COSMETIC_GAME_CALLS;}
typedef struct Award {
    Kh02GameGrantContext *game;
    Kh02CosmeticAttempt *attempt;
    Kh02Lease lease;
    Kh02AreaMapDescriptor descriptor;
} Award;
static uint32_t active(Kh02GameGrantContext *p) {
    return p && p->session && p->read && p->active && p->active(p->active_context)==1;
}
static uint32_t bytes(Kh02GameGrantContext *p,uint64_t a,void *out,uint32_t n) {
    return n && n<=64 && a>=0x10000 && a<=0x7fffffffffffULL-n && p->read(p->read_context,a,out,n)==1;
}
static uint32_t guard(Kh02GameGrantContext *p) {
    uint64_t vt,fn;unsigned char code[24];
    static const unsigned char award[]={0x85,0xd2,0x0f,0x84,0x96,0,0,0,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7c,0x24,0x18,0x41,0x56,0x48,0x83,0xec,0x20};
    static const unsigned char provider[]={0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x6c,0x24,0x20,0x56,0x48,0x83,0xec,0x20,0x48};
    static const unsigned char writer[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x81,0xc1,0xf4,0x0c,0,0,0x41,0x8b,0xd8,0xe8,0xab,0x77,0x01,0};
    if(!active(p) || p->base<0x10000 || p->base>0x7fffffffffffULL-0x04f5f000)return 0;
    if(!bytes(p,p->session->context,&vt,8) || !bytes(p,vt+0x428,&fn,8) || fn!=p->base+0x446aa0 ||
       !bytes(p,p->session->gate.current.provider,&vt,8) ||
       !bytes(p,vt+0x1500,&fn,8) || fn!=p->base+0x316fe0 ||
       !bytes(p,vt+0x8c0,&fn,8) || fn!=p->base+0x31e450)return 0;
    return bytes(p,p->base+0x446aa0,code,sizeof(award)) && !memcmp(code,award,sizeof(award)) &&
        bytes(p,p->base+0x316fe0,code,sizeof(provider)) && !memcmp(code,provider,sizeof(provider)) &&
        bytes(p,p->base+0x42af20,code,sizeof(writer)) && !memcmp(code,writer,sizeof(writer)) && active(p);
}
static uint32_t read_sample(Kh02GameGrantContext *p,Kh02SessionSample *s,Kh02CosmeticSnapshot *snapshot) {
    return active(p) && Kh02OwnerReadSample(p->read,p->read_context,p->base,p->cache,
        p->session->gate.current.generation,s)==KH02_GATE_OK && s->guards_verified==1 && !s->reserved &&
        s->phase==3 && !s->title_flags && !s->pending_map && !(s->travel_flags&1) && !s->world_flag &&
        Kh02ReadCosmeticSnapshot(p->read,p->read_context,s->world,snapshot)==KH02_GATE_OK &&
        snapshot->owner==s->context && active(p);
}
static uint32_t valid(Award *a,Kh02CosmeticSnapshot *snapshot) {
    Kh02GameGrantContext *p=a->game;Kh02SessionSample s;
    if(!active(p) || p->session->stopped || p->session->uncertain || !p->session->gate.ready ||
       !p->session->gate.busy || memcmp(&p->session->gate.current,&a->lease,sizeof(a->lease)) ||
       !read_sample(p,&s,snapshot) || s.generation!=a->lease.generation ||
       s.context!=p->session->context || s.world!=p->session->world ||
       s.state!=a->lease.state || s.provider!=a->lease.provider || s.attributes!=a->lease.attributes)return 0;
    /* Readers and guards are external callbacks: revalidate the lease after them. */
    return guard(p) && !p->session->stopped && !p->session->uncertain && p->session->gate.ready &&
        p->session->gate.busy && !memcmp(&p->session->gate.current,&a->lease,sizeof(a->lease));
}
static uint32_t effect(void *context) {
    Award *a=context;Kh02CosmeticAttempt *p=a->attempt;
    Kh02CosmeticSnapshot observed;
    if(!valid(a,&observed) || memcmp(&observed,&p->before,sizeof(observed)))return 0;
#if KH02_COSMETIC_GAME_CALLS
    /* Microsoft x64 ABI: context/code/amount/show in RCX/EDX/R8D/R9B,
       player 0 is the fifth stack argument. Return value is not an award ack.
       Use the native registration method, never the raw count writer. */
    typedef void (*NativeAward)(void*,uint32_t,int32_t,unsigned char,uint32_t);
    p->entered=1;
    ((NativeAward)(uintptr_t)(a->game->base+0x446aa0))(
        (void*)(uintptr_t)a->game->session->context,a->descriptor.code,1,1,0);
#else
    return 0;
#endif
    return valid(a,&p->after) && Kh02AreaMapFirstVerified(&p->before,&p->after,a->descriptor.item);
}
uint32_t Kh02AreaMapAwardOnce(Kh02GameGrantContext *game,uint64_t sequence,uint64_t item,Kh02CosmeticAttempt *attempt) {
    Kh02SessionSample sample;Award a;Kh02AreaMapDescriptor descriptor;
    if(!attempt || attempt->consumed)return KH02_GATE_REFUSED;
    /* This object is private to the verified owner thread. An invalid request
       or callback failure also consumes it; never turn refusal into a retry. */
    memset(attempt,0,sizeof(*attempt));attempt->consumed=1;attempt->sequence=sequence;
    attempt->status=KH02_GATE_REFUSED;
    if(!Kh02AreaMapResolve(item,&descriptor) || !sequence || !Kh02AreaMapAwardCallsEnabled() || !active(game) || game->session->gate.busy ||
       !read_sample(game,&sample,&attempt->before) || !Kh02AreaMapFirstEligible(&attempt->before,descriptor.item) ||
       Kh02SessionObserve(game->session,&sample)!=KH02_GATE_OK || !guard(game))return attempt->status;
    if(!Kh02AreaMapVerifyCode(game,item) || !active(game) || !guard(game))return attempt->status;
    a=(Award){game,attempt,game->session->gate.current,descriptor};
    sample.generation=a.lease.generation;
    attempt->status=Kh02SessionExecute(game->session,&a.lease,sequence,&sample,effect,&a);
    return attempt->status;
}
