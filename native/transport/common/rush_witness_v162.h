/* Completion events, never arena entry or inferred skipped rounds. */
#ifndef KH02_RUSH_WITNESS_V162_H
#define KH02_RUSH_WITNESS_V162_H
#include <stdint.h>
typedef struct RushWitness162 { uint64_t cookie,hits;uint32_t mask,reason; } RushWitness162;
static int rush_complete162(RushWitness162 *w,uint64_t cookie,unsigned round,
                            unsigned map,unsigned stage) {
    if(!cookie || round<1 || round>4 || map!=(round==1 || round==4?11u:12u) ||
       stage!=(round-1)*10)return 0;
    if(w->cookie!=cookie){w->cookie=cookie;w->mask=0;}
    w->mask|=1u<<(round-1);return 1;
}
#endif
