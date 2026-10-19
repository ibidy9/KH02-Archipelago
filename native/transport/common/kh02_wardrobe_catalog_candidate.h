#ifndef KH02_WARDROBE_CATALOG_CANDIDATE_H
#define KH02_WARDROBE_CATALOG_CANDIDATE_H
#include "kh02_cosmetic_award_candidate.h"
typedef struct Kh02WardrobeDescriptor {
    uint64_t item;
    uint32_t code,record;
} Kh02WardrobeDescriptor;
KH02_GATE_API uint32_t Kh02WardrobeResolve(uint64_t,Kh02WardrobeDescriptor*);
KH02_GATE_API uint32_t Kh02WardrobeFirstEligible(const Kh02CosmeticSnapshot*,uint64_t);
KH02_GATE_API uint32_t Kh02WardrobeFirstVerified(const Kh02CosmeticSnapshot*,const Kh02CosmeticSnapshot*,uint64_t);
KH02_GATE_API uint32_t Kh02WardrobeAwardOnce(Kh02GameGrantContext*,uint64_t,uint64_t,Kh02CosmeticAttempt*);
KH02_GATE_API uint32_t Kh02WardrobeAwardCallsEnabled(void);
KH02_GATE_API uint32_t Kh02WardrobeCatalogReceiptOnce(Kh02ReceiptOwner*,Kh02GameGrantContext*,uint64_t,const unsigned char*,uint32_t,Kh02CosmeticAttempt*);
#endif
