#ifndef KH02_METER_ACCESS_V48_H
#define KH02_METER_ACCESS_V48_H
#include <stdint.h>
typedef struct Kh02MeterAccess {
    uint64_t calls,applied,unchanged,refused,uncertain,no_ownership;
    uint64_t epoch,owner,magic,style,before,after,reference,status;
} Kh02MeterAccess;
void Kh02MeterReconcile(void);
void Kh02MeterAccessSnapshot(Kh02MeterAccess*);
#endif
