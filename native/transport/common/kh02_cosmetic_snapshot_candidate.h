#ifndef KH02_COSMETIC_SNAPSHOT_CANDIDATE_H
#define KH02_COSMETIC_SNAPSHOT_CANDIDATE_H
#include "kh02_owner_reader.h"

enum { KH02_COSMETIC_TEST_CODE=0x61d, KH02_COSMETIC_TEST_OBJECTIVE=13,
    KH02_COSMETIC_RECORD=0x13e + 0x1d, KH02_NATIVE_ITEM_RECORDS=0x310 };
typedef struct Kh02CosmeticBank {
    uint32_t objectives[51];
    uint32_t items[KH02_NATIVE_ITEM_RECORDS][2];
} Kh02CosmeticBank;
typedef struct Kh02CosmeticSnapshot {
    uint64_t owner;
    Kh02CosmeticBank primary,reference;
} Kh02CosmeticSnapshot;
/* Exact bounded reads only. Observational stability is NOT authorization to
   write. A separate native game-thread owner must establish its current lease,
   build/signature guards and a permanently consumed single-use request. */
KH02_GATE_API uint32_t Kh02ReadCosmeticSnapshot(Kh02BoundedRead,void*,uint64_t,Kh02CosmeticSnapshot*);
KH02_GATE_API uint32_t Kh02CosmeticFirstAwardEligible(const Kh02CosmeticSnapshot*);
/* Immediate same-callback comparison, before user browsing/equipping. Does not
   establish persistence, receipt membership, UI behavior or suppression. */
KH02_GATE_API uint32_t Kh02CosmeticFirstAwardVerified(const Kh02CosmeticSnapshot*,const Kh02CosmeticSnapshot*);
#endif
