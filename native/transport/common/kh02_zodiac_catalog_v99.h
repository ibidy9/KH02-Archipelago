#ifndef KH02_ZODIAC_CATALOG_CANDIDATE_H
#define KH02_ZODIAC_CATALOG_CANDIDATE_H
#include "kh02_cosmetic_award_candidate.h"
typedef struct Kh02ZodiacDescriptor {
    uint64_t item;
    uint32_t code,record;
} Kh02ZodiacDescriptor;
KH02_GATE_API uint32_t Kh02ZodiacResolve(uint64_t,Kh02ZodiacDescriptor*);
KH02_GATE_API uint32_t Kh02ZodiacFirstEligible(const Kh02CosmeticSnapshot*,uint64_t);
KH02_GATE_API uint32_t Kh02ZodiacFirstVerified(const Kh02CosmeticSnapshot*,const Kh02CosmeticSnapshot*,uint64_t);
KH02_GATE_API uint32_t Kh02ZodiacAwardOnce(Kh02GameGrantContext*,uint64_t,uint64_t,Kh02CosmeticAttempt*);
KH02_GATE_API uint32_t Kh02ZodiacAwardCallsEnabled(void);
KH02_GATE_API uint32_t Kh02ZodiacCatalogReceiptOnce(Kh02ReceiptOwner*,Kh02GameGrantContext*,uint64_t,const unsigned char*,uint32_t,Kh02CosmeticAttempt*);
uint64_t Kh02ZodiacPrepareRefresh(Kh02GameGrantContext*);
uint32_t Kh02ZodiacRefresh(Kh02GameGrantContext*,uint64_t);
#endif
