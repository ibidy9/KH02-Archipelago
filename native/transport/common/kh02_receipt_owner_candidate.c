#include "kh02_receipt_owner_candidate.h"
#include <string.h>
struct Kh02ReceiptOwner {
    Kh02MembershipAdmission admission;
    Kh02ReceiptBank *bank;
};
static uint64_t revision_of(const unsigned char *data){uint64_t n;memcpy(&n,data+32,8);return n;}
static int correct_thread(Kh02ReceiptOwner *o) {
    if(!o)return 0;
    if(o->admission.owner_thread==GetCurrentThreadId())return 1;
    Kh02MembershipInvalidate(&o->admission);return 0;
}
Kh02ReceiptOwner *Kh02ReceiptOwnerCreate(const Kh02ReceiptScope *scope,const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count,const unsigned char *baseline,uint32_t size) {
    Kh02ReceiptOwner *o=VirtualAlloc(NULL,sizeof(*o),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!o)return NULL;
    o->bank=Kh02ReceiptBankCreate(scope,history,count,ids,id_count);
    if(!o->bank || Kh02ReceiptBankBaseline(o->bank,baseline,size)!=KH02_BANK_OK ||
       Kh02MembershipInit(&o->admission,scope->cookie,revision_of(baseline))!=KH02_MEMBER_OK) {
        Kh02ReceiptBankDestroy(&o->bank);VirtualFree(o,0,MEM_RELEASE);return NULL;
    }
    return o;
}
uint32_t Kh02ReceiptOwnerHistory(Kh02ReceiptOwner *o,const Kh02ReceiptIdentity *history,uint32_t count) {
    if(!correct_thread(o))return KH02_MEMBER_WRONG_THREAD;
    if(InterlockedCompareExchange(&o->admission.revoked,0,0))return KH02_MEMBER_INVALID;
    /* An identical prefix or append changes availability, never applied membership. */
    if(Kh02ReceiptBankHistory(o->bank,history,count)==KH02_BANK_OK)return KH02_MEMBER_OK;
    Kh02MembershipInvalidate(&o->admission);return KH02_MEMBER_INVALID;
}
static uint32_t execute(Kh02ReceiptOwner *o,const unsigned char *proposal,uint32_t size,Kh02ReceiptEffect effect,Kh02ReceiptIdentityEffect identified,void *context) {
    Kh02MembershipTicket ticket={0};Kh02ReceiptIdentity added;uint32_t status,result;
    if(!o || (!effect && !identified))return KH02_MEMBER_REFUSED;
    status=Kh02MembershipBegin(&o->admission,KH02_MEMBER_EFFECT,&ticket);
    if(status!=KH02_MEMBER_OK)return status;
    /* Nothing that can invoke game code precedes admission and complete owned
       proposal registration. Allocation failures therefore have proven no effect. */
    if(!proposal || size<56 || revision_of(proposal)!=ticket.revision+1 ||
       Kh02ReceiptBankPropose(o->bank,ticket.revision,proposal,size)!=KH02_BANK_OK) {
        status=Kh02MembershipFinish(&o->admission,&ticket,KH02_MEMBER_NO_EFFECT);
        return status==KH02_MEMBER_OK?KH02_MEMBER_REFUSED:status;
    }
    if(identified && Kh02ReceiptBankAdded(o->bank,ticket.revision,ticket.revision+1,&added)!=KH02_BANK_OK) {
        status=Kh02MembershipFinish(&o->admission,&ticket,KH02_MEMBER_NO_EFFECT);
        return status==KH02_MEMBER_OK?KH02_MEMBER_REFUSED:status;
    }
    /* No bank lock spans the callback. Reentrant stages revoke admission and
       run normally in their future adapter, making this completion uncertain. */
    result=identified?identified(&added,ticket.cookie,context):effect(context);
    return Kh02MembershipFinish(&o->admission,&ticket,
        result==1?KH02_MEMBER_APPLIED:KH02_MEMBER_UNCERTAIN);
}
uint32_t Kh02ReceiptOwnerExecute(Kh02ReceiptOwner *o,const unsigned char *proposal,uint32_t size,Kh02ReceiptEffect effect,void *context) {
    return execute(o,proposal,size,effect,NULL,context);
}
uint32_t Kh02ReceiptOwnerExecuteIdentity(Kh02ReceiptOwner *o,const unsigned char *proposal,uint32_t size,Kh02ReceiptIdentityEffect effect,void *context) {
    return execute(o,proposal,size,NULL,effect,context);
}
uint32_t Kh02ReceiptOwnerCheck(Kh02ReceiptOwner *o) {
    if(!correct_thread(o))return KH02_MEMBER_WRONG_THREAD;
    return InterlockedCompareExchange(&o->admission.revoked,0,0) || o->admission.uncertain?KH02_MEMBER_INVALID:KH02_MEMBER_OK;
}
uint32_t Kh02ReceiptOwnerStageBegin(Kh02ReceiptOwner *o,Kh02MembershipTicket *ticket) {
    return o?Kh02MembershipBegin(&o->admission,KH02_MEMBER_STAGE,ticket):KH02_MEMBER_REFUSED;
}
uint32_t Kh02ReceiptOwnerStageFinish(Kh02ReceiptOwner *o,const Kh02MembershipTicket *ticket,uint32_t succeeded,Kh02ReceiptLease *out) {
    const Kh02ReceiptLease empty={0};uint32_t status;
    if(!o || !ticket || ticket->kind!=KH02_MEMBER_STAGE || !out || memcmp(out,&empty,sizeof(*out)))return KH02_MEMBER_REFUSED;
    status=Kh02MembershipFinish(&o->admission,ticket,succeeded==1?KH02_MEMBER_STAGE_OK:KH02_MEMBER_STAGE_FAILED);
    if(status!=KH02_MEMBER_OK)return status;
    if(Kh02ReceiptBankAcquire(o->bank,ticket->revision,out)==KH02_BANK_OK)return KH02_MEMBER_OK;
    Kh02MembershipInvalidate(&o->admission);return KH02_MEMBER_INVALID;
}
uint32_t Kh02ReceiptOwnerRead(Kh02ReceiptOwner *o,const Kh02ReceiptLease *lease,unsigned char *out,uint32_t size,uint32_t *written) {
    return o?Kh02ReceiptBankRead(o->bank,lease,out,size,written):KH02_BANK_REFUSED;
}
uint32_t Kh02ReceiptOwnerRelease(Kh02ReceiptOwner *o,Kh02ReceiptLease *lease) {
    return o?Kh02ReceiptBankRelease(o->bank,lease):KH02_BANK_REFUSED;
}
uint32_t Kh02ReceiptOwnerInvalidate(Kh02ReceiptOwner *o) {
    return o?Kh02MembershipInvalidate(&o->admission):KH02_MEMBER_REFUSED;
}
uint32_t Kh02ReceiptOwnerDestroy(Kh02ReceiptOwner **handle) {
    Kh02ReceiptOwner *o;uint32_t status;
    if(!handle)return KH02_BANK_REFUSED;o=*handle;if(!o)return KH02_BANK_OK;
    if(!correct_thread(o))return KH02_BANK_REFUSED;
    Kh02MembershipInvalidate(&o->admission);
    if(o->admission.busy)return KH02_BANK_BUSY;
    status=Kh02ReceiptBankDestroy(&o->bank);if(status!=KH02_BANK_OK)return status;
    if(!VirtualFree(o,0,MEM_RELEASE))return KH02_BANK_REFUSED;
    *handle=NULL;return KH02_BANK_OK;
}
