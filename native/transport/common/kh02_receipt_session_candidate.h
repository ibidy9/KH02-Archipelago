#ifndef KH02_RECEIPT_SESSION_CANDIDATE_H
#define KH02_RECEIPT_SESSION_CANDIDATE_H
#include "kh02_receipt_owner_candidate.h"
#include "kh02_session_candidate.h"
/* Fixed native consumable mapping; no caller-supplied battle-item selection.
   The local adapter callback must encompass the proven conversion, grant and
   post-validation and return 1 only on complete verified success. It must check
   OwnerCheck as well as session validity after conversion, before granting. */
typedef uint32_t (*Kh02ReceiptDispatch)(uint32_t,const Kh02ReceiptIdentity*,void*);
KH02_RECEIPT_API uint32_t Kh02ReceiptSessionExecute(Kh02ReceiptOwner*,Kh02SessionCandidate*,const Kh02Lease*,uint64_t,const Kh02SessionSample*,const unsigned char*,uint32_t,Kh02ReceiptDispatch,void*);
#endif
