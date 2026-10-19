#include "kh02_game_grant_candidate.h"
#include <string.h>
#ifndef KH02_GAME_GRANT_CALLS
#define KH02_GAME_GRANT_CALLS 0
#endif
uint32_t Kh02GameGrantCallsEnabled(void){return KH02_GAME_GRANT_CALLS;}
static uint32_t active(Kh02GameGrantContext *p) {
    return p && p->session && p->read && p->active && p->active(p->active_context)==1;
}
static uint32_t bytes(Kh02GameGrantContext *p,uint64_t address,void *out,uint32_t size) {
    return size && size<=64 && address>=0x10000 && address<=0x7fffffffffffULL-size &&
        p->read(p->read_context,address,out,size)==1;
}
static uint32_t guard(void *context) {
    Kh02GameGrantContext *p=context;uint64_t vt,fn;unsigned char code[14];
    static const unsigned char converter[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x41,0x8b,0xf8};
    static const unsigned char core[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x30,0x41,0x0f,0xb6,0xf8};
    static const unsigned char wrapper[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x30};
    if(!active(p) || p->base<0x10000 || p->base>0x7fffffffffffULL-0x04f5f000)return 0;
    /* Exact guards from the live-proven guarded Potion owner. */
    if(!bytes(p,p->session->context,&vt,8) || !bytes(p,vt+0x420,&fn,8) || fn!=p->base+0x446b40 ||
       !bytes(p,vt+0x428,&fn,8) || fn!=p->base+0x446aa0 ||
       !bytes(p,p->session->gate.current.provider,&vt,8) ||
       !bytes(p,vt+0x1500,&fn,8) || fn!=p->base+0x316fe0 ||
       !bytes(p,vt+0x8c0,&fn,8) || fn!=p->base+0x31e450)return 0;
    return bytes(p,p->base+0x42d6e0,code,sizeof(converter)) && !memcmp(code,converter,sizeof(converter)) &&
        bytes(p,p->base+0x42d7d0,code,sizeof(core)) && !memcmp(code,core,sizeof(core)) &&
        bytes(p,p->base+0x85c550,code,sizeof(wrapper)) && !memcmp(code,wrapper,sizeof(wrapper)) && active(p);
}
static uint32_t read_sample(void *context,Kh02SessionSample *sample,Kh02InventorySnapshot *inventory) {
    Kh02GameGrantContext *p=context;
    return active(p) && Kh02OwnerReadSample(p->read,p->read_context,p->base,p->cache,
        p->session->gate.current.generation,sample)==KH02_GATE_OK &&
        Kh02ReadInventory(p->read,p->read_context,sample->world,inventory)==KH02_GATE_OK && active(p);
}
static uint32_t convert(void *context,uint32_t battle,Kh02ReceiptItemName *name) {
    Kh02GameGrantContext *p=context;
#if KH02_GAME_GRANT_CALLS
    typedef Kh02ReceiptItemName *(*Convert)(Kh02ReceiptItemName*,void*,unsigned char);
    if(battle>6 || !guard(p))return 0;
    return ((Convert)(uintptr_t)(p->base+0x42d6e0))(name,NULL,(unsigned char)battle)==name && name->index && active(p);
#else
    (void)p;(void)battle;(void)name;return 0;
#endif
}
static void grant(void *context,const Kh02ReceiptItemName *name,uint32_t show) {
    Kh02GameGrantContext *p=context;
#if KH02_GAME_GRANT_CALLS
    typedef void (*Grant)(void*,Kh02ReceiptItemName,unsigned char);
    if(show<=1 && guard(p))((Grant)(uintptr_t)(p->base+0x42d7d0))(NULL,*name,(unsigned char)show);
#else
    (void)p;(void)name;(void)show;
#endif
}
uint32_t Kh02GameGrantPrepare(Kh02GameGrantContext *context,Kh02ReceiptOwner *owner,uint32_t show,
    Kh02ReceiptGrant *packet,Kh02SessionSample *sample) {
    Kh02InventorySnapshot inventory;
    if(!packet || !sample)return KH02_MEMBER_REFUSED;
    memset(packet,0,sizeof(*packet));memset(sample,0,sizeof(*sample));
    if(!owner || show>1 || !active(context) || !read_sample(context,sample,&inventory) ||
       Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK ||
       Kh02SessionObserve(context->session,sample)!=KH02_GATE_OK || !guard(context))return KH02_MEMBER_REFUSED;
    sample->generation=context->session->gate.current.generation;
    packet->owner=owner;packet->session=context->session;packet->lease=context->session->gate.current;
    packet->before=inventory;packet->show=show;packet->check=guard;packet->read=read_sample;
    packet->convert=convert;packet->grant=grant;packet->context=context;
    return KH02_MEMBER_OK;
}
