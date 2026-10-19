#ifndef KH02_POLICY_COMMAND_CANDIDATE_H
#define KH02_POLICY_COMMAND_CANDIDATE_H
#include "kh02_production_policy_candidate.h"
enum { KH02_POLICY_COMMAND=6, KH02_POLICY_MEMBERSHIP_CAP=102456 };
typedef struct Kh02PolicyCommand {
    uint32_t command,reserved;
    uint64_t sequence,live_authority,generation;
    Kh02PolicyWire policy;
    uint32_t membership_size,membership_reserved;
    unsigned char membership[KH02_POLICY_MEMBERSHIP_CAP];
} Kh02PolicyCommand;
typedef struct Kh02PolicyCommandOwner {
    Kh02PolicyState policy;
    uint64_t served;
    uint32_t thread,bound,faulted,reserved;
    Kh02PolicyApply apply;Kh02PolicyCommit commit;void *context;
} Kh02PolicyCommandOwner;
KH02_POLICY_API uint32_t Kh02PolicyCommandBind(Kh02PolicyCommandOwner*,uint64_t,uint64_t,
    const unsigned char*,Kh02PolicyApply,Kh02PolicyCommit,void*);
KH02_POLICY_API uint32_t Kh02PolicyCommandService(Kh02PolicyCommandOwner*,const Kh02PolicyCommand*);
#endif
