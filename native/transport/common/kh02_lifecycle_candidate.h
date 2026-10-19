#ifndef KH02_LIFECYCLE_CANDIDATE_H
#define KH02_LIFECYCLE_CANDIDATE_H
#include "kh02_disk_branch_candidate.h"
#include "kh02_checkpoint_receipts_candidate.h"
typedef struct Kh02Lifecycle Kh02Lifecycle;
KH02_RECEIPT_API Kh02Lifecycle *Kh02LifecycleCreate(const Kh02DiskLoadConfig*,const Kh02ReceiptScope*,
    const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t,const Kh02CheckpointConfig*);
KH02_RECEIPT_API unsigned char Kh02LifecycleLoad(Kh02Lifecycle*,uintptr_t,uint32_t);
KH02_RECEIPT_API uintptr_t Kh02LifecycleCapture(Kh02Lifecycle*,uintptr_t);
KH02_RECEIPT_API uintptr_t Kh02LifecycleRestore(Kh02Lifecycle*,uintptr_t,uint32_t);
KH02_RECEIPT_API Kh02ReceiptOwner *Kh02LifecycleCurrent(Kh02Lifecycle*);
KH02_RECEIPT_API uint64_t Kh02LifecycleEpoch(Kh02Lifecycle*);
KH02_RECEIPT_API uint32_t Kh02LifecycleStop(Kh02Lifecycle*);
KH02_RECEIPT_API uint32_t Kh02LifecycleDestroy(Kh02Lifecycle**);
#endif
