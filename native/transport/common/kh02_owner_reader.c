#include "kh02_owner_reader.h"
#include <string.h>
static const unsigned char getter[18]={0x48,0x8b,0x41,0x48,0x48,0x85,0xc0,0x74,0x08,
    0x48,0x8b,0x80,0x98,0x02,0,0,0xc3,0xc3};
static int bytes(Kh02BoundedRead f,void *u,uint64_t a,void *p,uint32_t n) {
    return f && a>=0x10000 && n && n<=64 && a<=0x7fffffffffffULL-n+1 && f(u,a,p,n)==1;
}
static int ptr(Kh02BoundedRead f,void *u,uint64_t a,uint64_t *p) {
    return bytes(f,u,a,p,8) && (!*p || (*p>=0x10000 && *p<=0x7fffffffffffULL-0x8000 && !(*p&7)));
}
uint32_t Kh02OwnerReadSample(Kh02BoundedRead f,void *u,uint64_t base,uint64_t cache,
        uint64_t epoch,Kh02SessionSample *out) {
    Kh02SessionSample s={0};uint64_t root,link,context,owner,world,vt,fn;
    uint64_t state,controller,provider,attributes,check;unsigned char code[18],flag;
    uint16_t title;
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(base<0x10000 || base>0x7fffffffffffULL-0x04f5f000 || !epoch ||
       cache<0x10000 || cache>0x7fffffffffffULL-0x8000 || (cache&7))return KH02_GATE_REFUSED;
#define P(a,v) if(!ptr(f,u,(a),&(v)) || !(v))return KH02_GATE_REFUSED
#define R(a,v) if(!bytes(f,u,(a),&(v),sizeof(v)))return KH02_GATE_REFUSED
#define SAME(a,v) if(!ptr(f,u,(a),&check) || check!=(v))return KH02_GATE_REFUSED
    P(base+0x480b5d0,root);P(root+0x230,link);P(link+0x50,context);
    P(context,vt);P(vt+0x138,fn);
    if(fn!=base+0x1dea690 || !bytes(f,u,fn,code,18) || memcmp(code,getter,18))return KH02_GATE_REFUSED;
    P(context+0x48,owner);P(owner+0x298,world);R(world+0xa54,flag);
    R(context+0x1b0,s.phase);R(context+0x7e14,title);
    R(context+0x7e88,s.pending_map);R(context+0x7ea0,s.travel_flags);
    P(cache+0x180,state);P(state+0x98,controller);P(controller+0x3e8,provider);
    P(provider+0x4c8,attributes);P(provider,vt);P(vt+0x1488,fn);
    if(fn!=base+0x31d470)return KH02_GATE_REFUSED;
    P(vt+0x1490,fn);if(fn!=base+0x31d490)return KH02_GATE_REFUSED;
    SAME(cache+0x180,state);SAME(state+0x98,controller);SAME(controller+0x3e8,provider);
    SAME(provider+0x4c8,attributes);SAME(context+0x48,owner);SAME(owner+0x298,world);
    SAME(base+0x480b5d0,root);SAME(root+0x230,link);SAME(link+0x50,context);
    s.generation=epoch;s.context=context;s.world=world;s.state=state;
    s.provider=provider;s.attributes=attributes;s.world_flag=flag;s.title_flags=title;
    s.guards_verified=1;*out=s;return KH02_GATE_OK;
#undef P
#undef R
#undef SAME
}
