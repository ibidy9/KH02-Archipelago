#ifndef KH02_CLEAR_CONTENT_CANDIDATE_H
#define KH02_CLEAR_CONTENT_CANDIDATE_H
#include "kh02_receipt_owner_candidate.h"
#define KH02_CLEAR_OK 1u
#define KH02_CLEAR_REFUSED 2u
KH02_RECEIPT_API uint32_t Kh02ClearContentInitialize(uintptr_t primary,uintptr_t checkpoint);
#endif
