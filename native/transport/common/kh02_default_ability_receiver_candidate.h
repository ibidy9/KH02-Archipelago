#ifndef KH02_DEFAULT_ABILITY_RECEIVER_CANDIDATE_H
#define KH02_DEFAULT_ABILITY_RECEIVER_CANDIDATE_H
#include <stdint.h>
#include "kh02_inventory_candidate.h"

typedef struct Kh02DefaultAbilityReceiver {
    uint64_t receiver,vtable,query,add,remove;
    uint32_t kind,reserved;
} Kh02DefaultAbilityReceiver;

/* File/build guard and a single observed receiver only. This never calls an
   ability function, changes memory, or searches the process for objects. */
KH02_GATE_API uint32_t Kh02DefaultAbilityInspect(Kh02BoundedRead,void*,
    uint64_t image_base,uint64_t receiver,uint32_t kind,
    Kh02DefaultAbilityReceiver *out);
#endif
