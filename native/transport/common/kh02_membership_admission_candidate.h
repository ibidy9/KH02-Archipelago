#ifndef KH02_MEMBERSHIP_ADMISSION_CANDIDATE_H
#define KH02_MEMBERSHIP_ADMISSION_CANDIDATE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
/* Offline component only. No hook, process reader, dispatcher or journal.
   Revision numbers name hypothetical immutable receipt sets, not quantities. */
typedef struct Kh02MembershipTicket { uint64_t cookie,sequence,kind,revision; } Kh02MembershipTicket;
typedef struct Kh02MembershipAdmission {
    uint64_t cookie,sequence,revision;
    uint32_t owner_thread;
    volatile LONG revoked;
    uint32_t busy,uncertain;
    Kh02MembershipTicket pending;
} Kh02MembershipAdmission;
enum { KH02_MEMBER_OK=1, KH02_MEMBER_REFUSED=2, KH02_MEMBER_INVALID=3, KH02_MEMBER_WRONG_THREAD=4 };
enum { KH02_MEMBER_EFFECT=1, KH02_MEMBER_STAGE=2 };
enum { KH02_MEMBER_APPLIED=1, KH02_MEMBER_NO_EFFECT=2, KH02_MEMBER_UNCERTAIN=3,
       KH02_MEMBER_STAGE_OK=4, KH02_MEMBER_STAGE_FAILED=5 };
#define MEMBER_API __declspec(dllexport)
MEMBER_API uint32_t Kh02MembershipInit(Kh02MembershipAdmission*,uint64_t,uint64_t);
MEMBER_API uint32_t Kh02MembershipBegin(Kh02MembershipAdmission*,uint32_t,Kh02MembershipTicket*);
MEMBER_API uint32_t Kh02MembershipFinish(Kh02MembershipAdmission*,const Kh02MembershipTicket*,uint32_t);
MEMBER_API uint32_t Kh02MembershipInvalidate(Kh02MembershipAdmission*);
#endif
