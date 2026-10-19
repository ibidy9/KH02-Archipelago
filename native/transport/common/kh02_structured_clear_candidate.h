#ifndef KH02_STRUCTURED_CLEAR_CANDIDATE_H
#define KH02_STRUCTURED_CLEAR_CANDIDATE_H
#include "kh02_receipt_owner_candidate.h"
/* Only the authenticated New Game owner may call this with the paired native
   structured-state pointers. This is not an arbitrary save-adoption API. */
KH02_RECEIPT_API uint32_t Kh02StructuredClearInitialize(uintptr_t,uintptr_t);
#endif
