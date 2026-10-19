#include "kh02_policy_command_candidate.h"
#include <windows.h>

uint32_t Kh02PolicyCommandBind(Kh02PolicyCommandOwner *owner,uint64_t owner_cookie,
        uint64_t process_cookie,const unsigned char *scope,Kh02PolicyApply apply,
        Kh02PolicyCommit commit,void *context) {
    if(!owner || owner->bound || !apply || !commit ||
       Kh02PolicyInit(&owner->policy,owner_cookie,process_cookie,scope)!=KH02_POLICY_OK)
        return KH02_POLICY_REFUSED;
    owner->thread=GetCurrentThreadId();owner->apply=apply;owner->commit=commit;
    owner->context=context;owner->bound=1;return KH02_POLICY_OK;
}

uint32_t Kh02PolicyCommandService(Kh02PolicyCommandOwner *owner,const Kh02PolicyCommand *command) {
    uint32_t status;
    if(!owner || !command || !owner->bound || owner->faulted ||
       GetCurrentThreadId()!=owner->thread || command->command!=KH02_POLICY_COMMAND ||
       command->reserved || command->membership_reserved || !command->sequence ||
       command->sequence!=owner->served+1)return KH02_POLICY_REFUSED;
    owner->served=command->sequence; /* consumed before any effect */
    status=Kh02PolicyTransaction(&owner->policy,&command->policy,command->live_authority,
        command->generation,command->membership,command->membership_size,
        owner->apply,owner->commit,owner->context);
    if(status==KH02_POLICY_UNCERTAIN)owner->faulted=1;
    return status;
}
