#ifndef KH02_TIDE_ACCESS_V77_H
#define KH02_TIDE_ACCESS_V77_H
#include "kh02_combined_route_candidate.h"

/* Internal adapter input, never a host-address/FName command. Names must be
 * resolved and scoped to the current process by the existing travel owner. */
typedef struct Kh02TideAccessV77 {
    uint64_t cookie,epoch,owner;
    uint64_t tide_map,tide_spawn,depths_map,save_spawn;
    uint32_t admitted,unlocked;
} Kh02TideAccessV77;

uint32_t Kh02TideAccessDecideV77(const Kh02TideAccessV77*,uint64_t,uint64_t,
    const char*,uint32_t,const Kh02RouteRequest*,Kh02RouteResult*);
#endif
