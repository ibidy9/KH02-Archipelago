#ifndef KH02_RECEIPT_OWNER_CANDIDATE_H
#define KH02_RECEIPT_OWNER_CANDIDATE_H
#include "kh02_receipt_bank_candidate.h"
#include "kh02_membership_admission_candidate.h"
typedef struct Kh02ReceiptOwner Kh02ReceiptOwner;
/* Synthetic adapter boundary only. Return 1 only after every existing native
   dispatcher/session check succeeds. Every other result is uncertain. The
   callback encompasses conversion and revalidation, not just the grant call. */
typedef uint32_t (*Kh02ReceiptEffect)(void*);
typedef uint32_t (*Kh02ReceiptIdentityEffect)(const Kh02ReceiptIdentity*,uint64_t,void*);
KH02_RECEIPT_API Kh02ReceiptOwner *Kh02ReceiptOwnerCreate(const Kh02ReceiptScope*,const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t,const unsigned char*,uint32_t);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerHistory(Kh02ReceiptOwner*,const Kh02ReceiptIdentity*,uint32_t);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerExecute(Kh02ReceiptOwner*,const unsigned char*,uint32_t,Kh02ReceiptEffect,void*);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerExecuteIdentity(Kh02ReceiptOwner*,const unsigned char*,uint32_t,Kh02ReceiptIdentityEffect,void*);
/* Owner-thread check for the adapter before/after reentrant native calls. */
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerCheck(Kh02ReceiptOwner*);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerStageBegin(Kh02ReceiptOwner*,Kh02MembershipTicket*);
/* A failed Begin never permits skipping the original game stage. Finish only
   an admitted ticket, after the original and exact record capture complete.
   On success, associate the returned lease with those captured record bytes. */
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerStageFinish(Kh02ReceiptOwner*,const Kh02MembershipTicket*,uint32_t,Kh02ReceiptLease*);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerRead(Kh02ReceiptOwner*,const Kh02ReceiptLease*,unsigned char*,uint32_t,uint32_t*);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerRelease(Kh02ReceiptOwner*,Kh02ReceiptLease*);
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerInvalidate(Kh02ReceiptOwner*);
/* Owner thread joins all API users before final destruction. Pending callbacks
   and retained stage receipts refuse destruction; invalidation does not erase them. */
KH02_RECEIPT_API uint32_t Kh02ReceiptOwnerDestroy(Kh02ReceiptOwner**);
#endif
