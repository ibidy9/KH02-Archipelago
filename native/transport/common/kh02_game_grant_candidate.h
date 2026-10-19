#ifndef KH02_GAME_GRANT_CANDIDATE_H
#define KH02_GAME_GRANT_CANDIDATE_H
#include "kh02_receipt_grant_candidate.h"
typedef struct Kh02GameGrantContext {
    uint64_t base,cache;
    Kh02SessionCandidate *session;
    Kh02BoundedRead read;
    void *read_context;
    Kh02GrantCheck active;
    void *active_context;
} Kh02GameGrantContext;
/* Internal owned configuration, never an untrusted mapped pointer packet.
   Retain configuration/session/reader through synchronous execution. */
KH02_RECEIPT_API uint32_t Kh02GameGrantPrepare(Kh02GameGrantContext*,Kh02ReceiptOwner*,uint32_t,Kh02ReceiptGrant*,Kh02SessionSample*);
KH02_RECEIPT_API uint32_t Kh02GameGrantCallsEnabled(void);
#endif
