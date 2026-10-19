#ifndef KH02_STAT_PROJECTION_V105_H
#define KH02_STAT_PROJECTION_V105_H
#include <stdint.h>
typedef struct Kh02StatView {
    uint64_t owner,attributes,epoch;
    uint32_t difficulty,base[2],maximum[2],current[2];
    unsigned char bonuses[32];
    uint32_t reserved;
} Kh02StatView;
typedef struct Kh02StatProjection {
    Kh02StatView before,after;
    uint32_t native[2],target[2],changed;
} Kh02StatProjection;
typedef int (*Kh02StatRead)(void*,Kh02StatView*);
typedef int (*Kh02StatWrite)(void*,const Kh02StatView*,const uint32_t*);
/* Returns 1 verified, 2 refused before mutation, 3 uncertain after mutation.
 * Caller owns the epoch/authentication checks and must fault on uncertainty. */
#ifdef KH02_STAT_SELFTEST
__declspec(dllexport)
#endif
uint32_t Kh02StatProject(Kh02StatRead,Kh02StatWrite,void*,uint32_t,uint32_t,uint32_t,uint32_t,Kh02StatProjection*);
#endif
