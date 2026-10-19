#ifndef KH02_CLEAR_LAYOUT_OBSERVATION_H
#define KH02_CLEAR_LAYOUT_OBSERVATION_H
#include "kh02_restore_snapshot_candidate.h"
#include "kh02_receipt_owner_candidate.h"
/* Exact structured-state fields only. No process search and no game writes.
   Result: summary[0..2], nonzero collectible-byte count, setting low31 bits,
   success bit63. Caller authenticates the fresh-copy state pointers. */
KH02_RECEIPT_API uint64_t Kh02ClearLayoutObserve(Kh02BoundedRead,void*,uintptr_t);
#endif
