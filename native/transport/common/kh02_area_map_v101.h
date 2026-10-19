#ifndef KH02_AREA_MAP_CATALOG_CANDIDATE_H
#define KH02_AREA_MAP_CATALOG_CANDIDATE_H
#include "kh02_cosmetic_award_candidate.h"
typedef struct Kh02AreaMapDescriptor {
    uint64_t item;
    uint32_t code,record;
} Kh02AreaMapDescriptor;
KH02_GATE_API uint32_t Kh02AreaMapResolve(uint64_t,Kh02AreaMapDescriptor*);
KH02_GATE_API uint32_t Kh02AreaMapFirstEligible(const Kh02CosmeticSnapshot*,uint64_t);
KH02_GATE_API uint32_t Kh02AreaMapFirstVerified(const Kh02CosmeticSnapshot*,const Kh02CosmeticSnapshot*,uint64_t);
KH02_GATE_API uint32_t Kh02AreaMapAwardOnce(Kh02GameGrantContext*,uint64_t,uint64_t,Kh02CosmeticAttempt*);
KH02_GATE_API uint32_t Kh02AreaMapAwardCallsEnabled(void);
KH02_GATE_API uint32_t Kh02AreaMapCatalogReceiptOnce(Kh02ReceiptOwner*,Kh02GameGrantContext*,uint64_t,const unsigned char*,uint32_t,Kh02CosmeticAttempt*);
uint32_t Kh02AreaMapVerifyCode(Kh02GameGrantContext*,uint64_t);
#endif
