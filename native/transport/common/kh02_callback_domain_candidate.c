#include "kh02_callback_domain_candidate.h"
void Kh02CallbackDomainInit(Kh02CallbackDomain *d) {
    ZeroMemory(d,sizeof(*d));InitializeSRWLock(&d->gate);d->owner=GetCurrentThreadId();
}
static uint32_t enter_locked(Kh02CallbackDomain *d,Kh02CallbackPin *pin) {
    if(d->readers==UINT64_MAX){d->closed=1;return 0;}
    ++d->readers;pin->domain=d;pin->selected=d->active;return 1;
}
uint32_t Kh02CallbackEnter(Kh02CallbackDomain *d,Kh02CallbackPin *pin) {
    uint32_t ok;if(!d || !pin || pin->domain)return 0;
    AcquireSRWLockExclusive(&d->gate);ok=enter_locked(d,pin);
    ReleaseSRWLockExclusive(&d->gate);return ok;
}
uint32_t Kh02CallbackLeave(Kh02CallbackPin *pin) {
    Kh02CallbackDomain *d;if(!pin || !(d=pin->domain))return 0;
    AcquireSRWLockExclusive(&d->gate);
    if(!d->readers){d->closed=1;ReleaseSRWLockExclusive(&d->gate);return 0;}
    --d->readers;pin->domain=NULL;pin->selected=NULL;
    ReleaseSRWLockExclusive(&d->gate);return 1;
}
uint32_t Kh02CallbackTransition(Kh02CallbackDomain *d,Kh02CallbackPin *pin,uint32_t *alone) {
    uint32_t ok;if(!d || !pin || pin->domain || !alone)return 0;
    *alone=0;AcquireSRWLockExclusive(&d->gate);ok=enter_locked(d,pin);
    if(ok){*alone=d->readers==1;d->active=NULL;}
    ReleaseSRWLockExclusive(&d->gate);return ok;
}
uint32_t Kh02CallbackPublish(Kh02CallbackDomain *d,void *active) {
    uint32_t ok;if(!d || !active || GetCurrentThreadId()!=d->owner)return 0;
    AcquireSRWLockExclusive(&d->gate);
    ok=!d->closed && !d->readers && !d->active;
    if(ok)d->active=active;ReleaseSRWLockExclusive(&d->gate);return ok;
}
uint32_t Kh02CallbackSnapshot(Kh02CallbackDomain *d,void **keep) {
    uint32_t ok;if(!d || !keep || GetCurrentThreadId()!=d->owner)return 0;
    *keep=NULL;AcquireSRWLockExclusive(&d->gate);ok=!d->readers;
    if(ok)*keep=d->active;ReleaseSRWLockExclusive(&d->gate);return ok;
}
uint32_t Kh02CallbackStop(Kh02CallbackDomain *d,Kh02CallbackPin *pin) {
    uint32_t ok;if(!d || !pin || pin->domain)return 0;
    AcquireSRWLockExclusive(&d->gate);d->closed=1;ok=enter_locked(d,pin);
    ReleaseSRWLockExclusive(&d->gate);return ok;
}
