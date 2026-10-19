#include "kh02_membership_admission_candidate.h"
#include <string.h>
_Static_assert(sizeof(Kh02MembershipTicket)==32,"ticket ABI");
_Static_assert(sizeof(Kh02MembershipAdmission)==72,"admission ABI");
static int owner(Kh02MembershipAdmission *s) {
    if(s->owner_thread==GetCurrentThreadId())return 1;
    InterlockedOr(&s->revoked,4);return 0;
}
uint32_t Kh02MembershipInit(Kh02MembershipAdmission *s,uint64_t cookie,uint64_t revision) {
    const Kh02MembershipAdmission empty={0};
    /* Caller owns unpublished zero storage. Never reset an existing owner. */
    if(!s || !cookie || !revision || memcmp(s,&empty,sizeof(*s)))return KH02_MEMBER_REFUSED;
    s->cookie=cookie;s->revision=revision;s->owner_thread=GetCurrentThreadId();
    return KH02_MEMBER_OK;
}
uint32_t Kh02MembershipBegin(Kh02MembershipAdmission *s,uint32_t kind,Kh02MembershipTicket *out) {
    const Kh02MembershipTicket empty={0};
    if(!s || !out || !s->cookie || memcmp(out,&empty,sizeof(*out)) ||
       (kind!=KH02_MEMBER_EFFECT && kind!=KH02_MEMBER_STAGE))return KH02_MEMBER_REFUSED;
    if(!owner(s))return KH02_MEMBER_WRONG_THREAD;
    if(InterlockedCompareExchange(&s->revoked,0,0) || s->uncertain)return KH02_MEMBER_INVALID;
    if(s->busy) {
        /* Original game construction must still run. Its resulting association
           is unavailable; never wait or suppress the original stage callback. */
        if(kind==KH02_MEMBER_STAGE)InterlockedOr(&s->revoked,2);
        return KH02_MEMBER_REFUSED;
    }
    if(s->sequence==UINT64_MAX || (kind==KH02_MEMBER_EFFECT && s->revision==UINT64_MAX)) {
        InterlockedOr(&s->revoked,16);return KH02_MEMBER_INVALID;
    }
    s->pending=(Kh02MembershipTicket){s->cookie,++s->sequence,kind,s->revision};
    s->busy=1;*out=s->pending;return KH02_MEMBER_OK;
}
uint32_t Kh02MembershipFinish(Kh02MembershipAdmission *s,const Kh02MembershipTicket *ticket,uint32_t result) {
    uint32_t invalid;
    if(!s || !ticket || !s->cookie)return KH02_MEMBER_REFUSED;
    if(!owner(s))return KH02_MEMBER_WRONG_THREAD;
    if(!s->busy || memcmp(ticket,&s->pending,sizeof(*ticket)))return KH02_MEMBER_REFUSED;
    if((ticket->kind==KH02_MEMBER_EFFECT && (result<1 || result>3)) ||
       (ticket->kind==KH02_MEMBER_STAGE && result!=4 && result!=5))return KH02_MEMBER_REFUSED;
    /* Revocation invalidates the result, not the obligation to drain the exact
       original call. A stale ticket cannot drain another outstanding call. */
    if(result==KH02_MEMBER_UNCERTAIN || result==KH02_MEMBER_STAGE_FAILED) {
        s->uncertain=1;InterlockedOr(&s->revoked,8);
    }
    invalid=InterlockedCompareExchange(&s->revoked,0,0)!=0;
    if(!invalid && result==KH02_MEMBER_APPLIED)s->revision++;
    memset(&s->pending,0,sizeof(s->pending));s->busy=0;
    return invalid?KH02_MEMBER_INVALID:KH02_MEMBER_OK;
}
uint32_t Kh02MembershipInvalidate(Kh02MembershipAdmission *s) {
    if(!s || !s->cookie)return KH02_MEMBER_REFUSED;
    /* May be signalled from another thread; never mutate owner-only state. */
    InterlockedOr(&s->revoked,1);return KH02_MEMBER_OK;
}
