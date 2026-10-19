#ifndef KH02_METER_PROJECTION_V48_H
#define KH02_METER_PROJECTION_V48_H
#include "kh02_inventory_candidate.h"
typedef int (*Kh02MeterWrite)(void*,uint64_t,const void*,uint32_t);
typedef struct Kh02MeterProjection {
    uint32_t before,after,reference,changed;
} Kh02MeterProjection;
#ifdef KH02_METER_SELFTEST
__declspec(dllexport)
#endif
uint32_t Kh02MeterProject(Kh02BoundedRead,Kh02MeterWrite,void*,uint64_t,uint64_t,Kh02MeterProjection*);
#endif
