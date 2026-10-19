#ifndef KH02_GRANT_OWNER_CANDIDATE_H
#define KH02_GRANT_OWNER_CANDIDATE_H
#include "kh02_lifecycle_grant_candidate.h"
#include "kh02_game_grant_candidate.h"
#include "kh02_default_ability_receiver_candidate.h"
typedef struct Kh02GrantOutcome {
    uint64_t epoch,nonce;
    uint32_t status,entered,before,after,consumed,reserved;
} Kh02GrantOutcome;
#include "kh02_action_observer_v32.h"
/* One retained owner per process; caller retains lifecycle until process exit.
   These internal APIs are not mapped request pointer fields. */
KH02_RECEIPT_API uint32_t Kh02GrantOwnerArm(uintptr_t,Kh02Lifecycle*,uint64_t,uint64_t);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerProbe(uint64_t,Kh02GrantOutcome*);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerExecute(uint64_t,uint64_t,uint32_t,const unsigned char*,uint32_t,Kh02GrantOutcome*);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerStop(void);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerFlight(void);
KH02_RECEIPT_API uint32_t Kh02GrantOwnerDefaultAbilitySnapshot(Kh02DefaultAbilitySnapshot*);
#endif
