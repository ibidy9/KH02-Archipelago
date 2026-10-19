#ifndef KH02_OPENING_TRACE_V54_H
#define KH02_OPENING_TRACE_V54_H
#include <stdint.h>
void Kh02OpeningTraceCopy(uint32_t phase,uintptr_t base,uintptr_t caller,
                          uintptr_t destination,uintptr_t source);
void Kh02OpeningTraceRoute(uintptr_t base,uintptr_t caller,uintptr_t owner,
                           uint64_t map,uint64_t spawn,uint32_t fade);
void Kh02OpeningTraceStart(uintptr_t base,uintptr_t caller,uintptr_t owner,
                          uint32_t phase,uint32_t reason);
void Kh02OpeningTraceRemote(uintptr_t base,uintptr_t owner,uintptr_t context,uint64_t event,uint32_t reason);
void Kh02OpeningTraceOriginal(uintptr_t base,uintptr_t caller,const uint64_t *frame);
#endif
