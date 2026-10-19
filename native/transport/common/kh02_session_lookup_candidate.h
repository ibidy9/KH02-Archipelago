#ifndef KH02_SESSION_LOOKUP_CANDIDATE_H
#define KH02_SESSION_LOOKUP_CANDIDATE_H
#include "kh02_disk_lookup_candidate.h"
/* Additive implementation of the disk-lookup ABI. This extra entry is for a
   trusted native publication adapter ONLY, after durable association succeeds.
   It is not a transport command, save adoption, or a durability operation.
   Source epoch must match the exact receipt envelope. Revision/epoch are
   normalized for future load admission; AP identity and membership are exact.
   Allocation/conflicting evidence closes admission. No proof is evicted.
   Caller retains/excludes users for the lifetime of the lookup. */
KH02_RECEIPT_API uint32_t Kh02SessionLookupRemember(Kh02DiskLookup*,uint64_t,
    const unsigned char*,uint32_t,const unsigned char*,uint32_t);
#endif
