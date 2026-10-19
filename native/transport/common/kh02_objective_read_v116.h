#ifndef KH02_OBJECTIVE_READ_V116_H
#define KH02_OBJECTIVE_READ_V116_H
#include "kh02_owner_reader.h"
typedef struct Kh02ObjectiveStatesV116 {
    uint64_t owner;
    uint32_t states[51];
} Kh02ObjectiveStatesV116;
/* Caller supplies its admitted context/world and independently validates the
   lease before/after. This function is read-only and cannot authorize reports. */
KH02_GATE_API uint32_t Kh02ObjectiveReadV116(Kh02BoundedRead,void*,uint64_t,
    uint64_t,uint64_t,Kh02ObjectiveStatesV116*);
#endif
