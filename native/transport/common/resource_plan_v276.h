#ifndef RESOURCE_PLAN_V276_H
#define RESOURCE_PLAN_V276_H
#include "resource_plan_v211.h"
static uint32_t resource_plan276(const Kh02StatView *source,
 uint32_t source_hp,uint32_t source_mp,const Kh02StatProjection *p,
 uint32_t hp,uint32_t mp,uint32_t disabled_castle_magic,uint32_t target[2]) {
 if(resource_plan211(source,source_hp,source_mp,p,hp,mp,target))return 1;
 if(!source || !p || !target || !disabled_castle_magic ||
    source->attributes==p->before.attributes || p->before.current[1] || p->after.current[1])return 0;
 Kh02StatProjection normalized=*p;
 normalized.before.current[1]=source->current[1]<p->native[1]?source->current[1]:p->native[1];
 if(!resource_plan211(source,source_hp,source_mp,&normalized,hp,mp,target))return 0;
 /* MP stays disabled here. The separately guarded native magic initializer
  * owns MP setup. HP restores exactly the captured amount, never a free heal. */
 target[1]=p->after.current[1];return 1;
}
#endif
