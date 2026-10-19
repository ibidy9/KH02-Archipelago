/* Pure composition of the existing route decision. No hooks, reads or writes. */
#include "tide_access_v77.h"
#include <string.h>

uint32_t Kh02TideAccessDecideV77(const Kh02TideAccessV77 *a,uint64_t owner,uint64_t epoch,
    const char *source_map,uint32_t story,const Kh02RouteRequest *q,Kh02RouteResult *out) {
    Kh02RoutePolicy policy={0};Kh02RouteRule *rule=&policy.rules[0];
    if(!q || !out)return KH02_INVALID;
    out->map=q->map;out->spawn=q->spawn;out->fade=q->fade;out->rule_index=UINT32_MAX;
    /* Generic trigger callback is not by itself a Tide discriminator. */
    if(q->caller_rva!=0x563c95)return KH02_ALLOW;
    if(!a || !a->cookie || !a->epoch || !a->owner || a->admitted!=1 || a->unlocked>1 ||
       !a->tide_map || !a->tide_spawn || !a->depths_map || !a->save_spawn ||
       a->tide_map==a->depths_map)return KH02_INVALID;
    if(a->cookie!=q->owner_cookie || a->epoch!=epoch || q->generation!=epoch ||
       a->owner!=owner)return KH02_STALE;
    if(q->map!=a->tide_map || q->spawn!=a->tide_spawn)return KH02_ALLOW;
    /* Caller provides a bounded zero-terminated 32-byte map snapshot. Compare
     * the full literal including terminator so prefixes cannot match. */
    if(!source_map || memcmp(source_map,"/Game/Levels/dw/dw_09/dw_09",sizeof("/Game/Levels/dw/dw_09/dw_09")) || story!=410)
        return KH02_INVALID;
    policy.owner_cookie=a->cookie;policy.generation=a->epoch;policy.count=1;
    rule->kind=KH02_DEMON_TIDE;rule->caller_rva=0x563c95;rule->enabled=!a->unlocked;
    rule->source_map=a->tide_map;rule->source_spawn=a->tide_spawn;
    rule->target_map=a->depths_map;rule->target_spawn=a->save_spawn;
    return Kh02DecideRoute(&policy,q,out);
}
