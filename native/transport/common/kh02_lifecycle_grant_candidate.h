#ifndef KH02_LIFECYCLE_GRANT_CANDIDATE_H
#define KH02_LIFECYCLE_GRANT_CANDIDATE_H
#include "kh02_lifecycle_candidate.h"
#include "kh02_receipt_grant_candidate.h"
/* Caller retains life, session, callbacks and output through completion. No
   discovery, history mutation, automatic replay or live command is supplied. */
KH02_RECEIPT_API uint32_t Kh02LifecycleGrant(Kh02Lifecycle*,uint64_t,uint64_t,
    const Kh02SessionSample*,const unsigned char*,uint32_t,Kh02ReceiptGrant*);
KH02_RECEIPT_API uint32_t Kh02LifecycleLocalGrant193(Kh02Lifecycle*,uint64_t,uint32_t,Kh02ReceiptGrant*);
#endif
