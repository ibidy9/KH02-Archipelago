#ifndef KH02_COSMETIC_AWARD_CANDIDATE_H
#define KH02_COSMETIC_AWARD_CANDIDATE_H
#include "kh02_game_grant_candidate.h"
#include "kh02_cosmetic_snapshot_candidate.h"

typedef struct Kh02CosmeticAttempt {
    uint32_t consumed,entered,status,reserved;
    uint64_t sequence;
    Kh02CosmeticSnapshot before,after;
} Kh02CosmeticAttempt;
/* Private, zero-initialized owner storage, never an untrusted mapped packet.
   The caller must supply the proven game-thread active callback and exact-build
   reader, retain the session and this attempt until game exit, and durably
   reserve the attempt BEFORE dispatch. Reusing/clearing it is forbidden.
   This isolated experiment creates no AP receipt association. */
KH02_GATE_API uint32_t Kh02CosmeticAwardOnce(Kh02GameGrantContext*,uint64_t,Kh02CosmeticAttempt*);
KH02_GATE_API uint32_t Kh02CosmeticAwardCallsEnabled(void);
#endif
