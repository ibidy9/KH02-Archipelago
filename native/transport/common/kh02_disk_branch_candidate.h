#ifndef KH02_DISK_BRANCH_CANDIDATE_H
#define KH02_DISK_BRANCH_CANDIDATE_H
#include "kh02_disk_load_candidate.h"
#include "kh02_receipt_owner_candidate.h"
typedef struct Kh02DiskBranch Kh02DiskBranch;
/* Offline composition. Copies scope/history/support; borrows the immutable
   lookup, original and read callbacks until all invocations are joined. */
KH02_RECEIPT_API Kh02DiskBranch *Kh02DiskBranchCreate(const Kh02DiskLoadConfig*,const Kh02ReceiptScope*,
    const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t);
KH02_RECEIPT_API unsigned char Kh02DiskBranchInvoke(Kh02DiskBranch*,uintptr_t,uint32_t);
/* Borrowed owner; all effect/stage users must also check current branch
   freshness. Previously returned owners remain allocated but invalidated. */
KH02_RECEIPT_API Kh02ReceiptOwner *Kh02DiskBranchCurrent(Kh02DiskBranch*);
KH02_RECEIPT_API uint32_t Kh02DiskBranchDestroy(Kh02DiskBranch**);
#endif
