#ifndef KH02_STYLE_GATE_V47_H
#define KH02_STYLE_GATE_V47_H
#include <stdint.h>
typedef struct Kh02StyleGate {
    uint64_t raw,reasons[7],accepted[2],blocked[2];
    uint64_t epoch,controller,actor,command,caller,owned;
} Kh02StyleGate;
void Kh02StyleGateSnapshot(Kh02StyleGate*);
uint32_t Kh02StyleOwnership(uint32_t*);
uint32_t Kh02StyleReceiptReady(uint64_t);
#endif
