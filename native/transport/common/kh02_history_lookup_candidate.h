#ifndef KH02_HISTORY_LOOKUP_CANDIDATE_H
#define KH02_HISTORY_LOOKUP_CANDIDATE_H
#include "kh02_session_lookup_candidate.h"
/* Internal trusted coordinator callback: bounded memory operations only, no
   game calls, blocking waits, journal I/O or recursion into this lookup.
   Any callback failure closes lookup admission; a partial commit cannot be
   mistaken for an unchanged BUSY result. Caller closes the remaining graph. */
typedef uint32_t (*Kh02HistoryCommit)(void*,const Kh02ReceiptIdentity*,uint32_t,uint32_t);
KH02_RECEIPT_API uint32_t Kh02SessionLookupHistory(Kh02DiskLookup*,const Kh02ReceiptScope*,
    const Kh02ReceiptIdentity*,uint32_t,Kh02HistoryCommit,void*);
#endif
