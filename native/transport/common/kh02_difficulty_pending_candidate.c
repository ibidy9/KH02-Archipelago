#include "kh02_difficulty_pending_candidate.h"
#include <string.h>
_Static_assert(sizeof(Kh02DifficultyRequest)==88,"difficulty request ABI");
_Static_assert(sizeof(Kh02DifficultyPending)==96,"difficulty state ABI");
uint32_t Kh02DifficultyBind(Kh02DifficultyPending *s,uint64_t cookie,uint64_t challenge,const unsigned char *scope) {
    if(!s || s->cookie || !cookie || !challenge || !scope)return 2;
    memset(s,0,sizeof(*s));s->cookie=cookie;s->challenge=challenge;memcpy(s->scope,scope,32);return 1;
}
uint32_t Kh02DifficultyQueue(Kh02DifficultyPending *s,const Kh02DifficultyRequest *r,
        uint64_t authority,uint64_t epoch,uint32_t permitted) {
    if(!s || !r || !s->cookie || permitted!=1 || !authority || !epoch ||
       r->magic!=0x4b484446 || r->version!=1 || r->size!=sizeof(*r) || r->desired>3 ||
       !r->sequence || r->sequence<=s->sequence || r->cookie!=s->cookie ||
       r->challenge!=s->challenge || r->authority!=authority || r->epoch!=epoch ||
       memcmp(r->scope,s->scope,32))return 2;
    s->sequence=r->sequence;s->authority=authority;s->requested_epoch=epoch;
    s->applied_epoch=0;s->desired=r->desired;s->phase=1;s->result=0;return 1;
}
uint32_t Kh02DifficultyApply(Kh02DifficultyPending *s,uint64_t authority,uint64_t epoch,
        uint32_t permitted,uintptr_t context,uint32_t expected,Kh02DifficultyWrite write) {
    if(!s || !s->cookie || s->phase!=1)return 0;
    /* Unknown/title loads never consume the pending request. */
    if(permitted!=1 || !epoch || epoch<=s->requested_epoch)return 0;
    if(!authority || authority!=s->authority || !context || expected>3 || !write) {
        s->phase=4;s->result=2;return 2;
    }
    s->result=write(context,expected,s->desired);
    s->phase=s->result==1?2:4;s->applied_epoch=s->result==1?epoch:0;
    return s->result;
}
uint32_t Kh02DifficultyConfirm(Kh02DifficultyPending *s,uint64_t authority,uint64_t epoch,
        uint32_t permitted,uint64_t observed) {
    if(!s || s->phase!=2 || permitted!=1 || !authority || authority!=s->authority ||
       epoch!=s->applied_epoch || observed!=(1|((uint64_t)s->desired<<8)|((uint64_t)s->desired<<16)))return 0;
    s->phase=3;return 1;
}
