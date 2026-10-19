#ifndef KH02_CONTROLLED_OWNER_CANDIDATE_H
#define KH02_CONTROLLED_OWNER_CANDIDATE_H
#include "kh02_mixed_delivery_candidate.h"
#include "kh02_lifecycle_candidate.h"
KH02_RECEIPT_API uint32_t Kh02ControlledOwnerBind(Kh02SaveRouter*,Kh02GrantCheck,void*);
KH02_RECEIPT_API uint32_t Kh02ControlledOwnerExecute(uint64_t,uint64_t,uint32_t,const unsigned char*,uint32_t,Kh02MixedOutcome*);
KH02_RECEIPT_API uint32_t Kh02ControlledOwnerProbe(uint64_t*,unsigned char*,uint32_t,uint32_t*);
KH02_RECEIPT_API uint32_t Kh02MixedDeliveryProbe(Kh02MixedDelivery*,uint64_t*,unsigned char*,uint32_t,uint32_t*);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerArm(uintptr_t,Kh02Lifecycle*,uint64_t,uint64_t);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerStop(void);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerFlight(void);
#endif
