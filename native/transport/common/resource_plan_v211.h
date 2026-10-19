#ifndef RESOURCE_PLAN_V211_H
#define RESOURCE_PLAN_V211_H
#include "kh02_stat_projection_v105.h"
#include <string.h>
/* Planning only. Caller supplies a verified, same-epoch travel snapshot.
 * Item arrivals, load epochs and unexplained current-value changes cannot heal. */
static uint32_t resource_plan211(const Kh02StatView *source,
        uint32_t source_hp,uint32_t source_mp,const Kh02StatProjection *p,
        uint32_t hp,uint32_t mp,uint32_t target[2]) {
    if(!source || !p || !target || !source->owner || !source->epoch ||
       source->owner!=p->before.owner || source->epoch!=p->before.epoch ||
       source->difficulty!=p->before.difficulty || source_hp!=hp || source_mp!=mp ||
       memcmp(source->base,p->before.base,8) || memcmp(source->bonuses,p->before.bonuses,32) ||
       !source->current[0] || !p->before.current[0] ||
       memcmp(source->maximum,p->target,8) || memcmp(p->before.maximum,p->native,8) ||
       memcmp(p->after.maximum,p->target,8))return 0;
    for(uint32_t i=0;i<2;i++) {
        uint32_t clamped=source->current[i]<p->native[i]?source->current[i]:p->native[i];
        if(source->current[i]>source->maximum[i] || p->before.current[i]>p->native[i] ||
           (p->before.current[i]!=clamped && p->before.current[i]!=p->native[i]))return 0;
        target[i]=source->current[i];
    }
    return 1;
}
#endif
