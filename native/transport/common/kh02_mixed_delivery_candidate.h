#ifndef KH02_MIXED_DELIVERY_CANDIDATE_H
#define KH02_MIXED_DELIVERY_CANDIDATE_H
#include "kh02_wardrobe_catalog_candidate.h"
typedef struct Kh02SaveRouter Kh02SaveRouter;
typedef struct Kh02MixedDelivery Kh02MixedDelivery;
enum {KH02_MIXED_CONSUMABLE=1,KH02_MIXED_WARDROBE=2,KH02_MIXED_ZODIAC=3,KH02_MIXED_AREA_MAP=4};
typedef struct Kh02MixedOutcome {
    uint64_t epoch,sequence;
    uint32_t status,kind,entered,before,after,reserved;
} Kh02MixedOutcome;
/* Internal trusted owner-thread configuration, NOT a mapped command or AP
   authorization. Retain router, game context/session/callbacks and this object through
   process exit. Attempts are individually retained and never reused. */
KH02_RECEIPT_API Kh02MixedDelivery *Kh02MixedDeliveryCreate(Kh02SaveRouter*,const Kh02GameGrantContext*);
KH02_RECEIPT_API uint32_t Kh02MixedDeliveryExecute(Kh02MixedDelivery*,uint64_t,uint64_t,uint32_t,
    const unsigned char*,uint32_t,Kh02MixedOutcome*);
#endif
