#ifndef KH02_DISK_LOOKUP_CANDIDATE_H
#define KH02_DISK_LOOKUP_CANDIDATE_H
#include "kh02_receipt_bank_candidate.h"
typedef struct Kh02DiskLookup Kh02DiskLookup;
typedef struct Kh02DiskEntry {
    const unsigned char *record;
    uint32_t record_size,reserved;
    const unsigned char *receipts;
    uint32_t receipt_size,reserved2;
} Kh02DiskEntry;
/* Offline immutable lookup, NOT proof that native load occurred. Scope tags
   identify this table's lifetime, never a slot or an inferred restored branch.
   Inputs are owned local buffers. At most 64 entries, receipt revision exactly 1.
   Caller establishes journal durability/provenance and complete server history. */
KH02_RECEIPT_API Kh02DiskLookup *Kh02DiskLookupCreate(const Kh02ReceiptScope*,
    const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t,const Kh02DiskEntry*,uint32_t);
/* Copies the validated receipt set. Unknown bytes return REFUSED, never empty.
   No game access, SQLite, host wait, allocation, or item delivery. */
KH02_RECEIPT_API uint32_t Kh02DiskLookupResolve(Kh02DiskLookup*,uint64_t,uint64_t,
    const unsigned char*,uint32_t,unsigned char*,uint32_t,uint32_t*);
/* Creating thread only. Caller excludes/joins all users before final free;
   failed try-lock returns BUSY and retains storage. */
KH02_RECEIPT_API uint32_t Kh02DiskLookupDestroy(Kh02DiskLookup**);
#endif
