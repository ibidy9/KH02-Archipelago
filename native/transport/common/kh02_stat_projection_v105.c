#include "kh02_stat_projection_v105.h"
#include <string.h>
static int pointer(uint64_t p){return p>=0x10000 && p<=0x7fffffffffffULL-0x8000 && !(p&7);}
uint32_t Kh02StatProject(Kh02StatRead read,Kh02StatWrite write,void *context,
        uint32_t hp,uint32_t mp,uint32_t previous_hp,uint32_t previous_mp,Kh02StatProjection *out) {
    static const uint32_t values[4][4]={{5,10,10,40},{5,10,10,40},{3,6,6,26},{2,5,5,20}};
    Kh02StatView again,expected;uint64_t native[2],target[2];uint32_t prior[2];
    if(!out)return 2;
    memset(out,0,sizeof(*out));memset(&again,0,sizeof(again));
    if(!read || !write || hp>4096 || mp>4096 || previous_hp>hp || previous_mp>mp ||
       !read(context,&out->before))return 2;
    const Kh02StatView *s=&out->before;
    if(!pointer(s->owner) || !pointer(s->attributes) || !s->epoch || s->difficulty>3 || s->reserved)return 2;
    native[0]=s->base[0];native[1]=s->base[1];
    for(uint32_t i=0;i<32;i++) {
        uint32_t code=s->bonuses[i];if(code>4)return 2;
        if(code)native[code>=3]+=values[s->difficulty][code-1];
    }
    target[0]=native[0]+5ULL*hp;target[1]=native[1]+10ULL*mp;
    if(native[0]<20 || target[0]>500 || target[1]>200)return 2;
    /* A native recalculation can reset maxima one frame before clamping current.
       Restore only already-earned maxima; never write current or borrow a snapshot.
       New receipts, mixed/unexplained maxima, and out-of-target current still refuse. */
    if(s->current[0]>s->maximum[0] || s->current[1]>s->maximum[1]) {
        if(hp!=previous_hp || mp!=previous_mp ||
           s->maximum[0]!=native[0] || s->maximum[1]!=native[1] ||
           s->current[0]>target[0] || s->current[1]>target[1])return 2;
    }
    for(uint32_t i=0;i<2;i++){out->native[i]=(uint32_t)native[i];out->target[i]=(uint32_t)target[i];}
    prior[0]=out->native[0]+5*previous_hp;prior[1]=out->native[1]+10*previous_mp;
    if(memcmp(s->maximum,out->native,8) && memcmp(s->maximum,out->target,8) && memcmp(s->maximum,prior,8))return 2;
    if(!read(context,&again) || memcmp(s,&again,sizeof(again)))return 2;
    if(!memcmp(s->maximum,out->target,8)){out->after=*s;return 1;}
    int written=write(context,s,out->target);
    if(!read(context,&out->after))return 3;
    if(!written && !memcmp(s,&out->after,sizeof(*s)))return 2;
    expected=*s;memcpy(expected.maximum,out->target,8);
    if(!written || memcmp(&expected,&out->after,sizeof(expected)))return 3;
    out->changed=1;return 1;
}
