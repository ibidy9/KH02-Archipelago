#include "kh02_fresh_replay_router_candidate.c"
#include "kh02_history_lookup_candidate.h"
#ifdef KH02_FRESH_SETUP_SELFTEST
static uint32_t history_test_fault;
MEMBER_API void Kh02HistoryRouterTestFault(uint32_t value) {history_test_fault=value;}
#endif
static uint32_t history_commit(void *context,const Kh02ReceiptIdentity *history,uint32_t before,uint32_t count) {
    Kh02SaveRouter *p=context;Kh02DiskBranch *branch=p->life->branch;Kh02ReceiptOwner *owner;
    if(p->closed || p->life->closed || branch->closed || branch->history_count!=before ||
       (before && memcmp(branch->history,history,before*sizeof(*history))))return KH02_BANK_REFUSED;
#ifdef KH02_FRESH_SETUP_SELFTEST
    if(history_test_fault==1)return KH02_BANK_REFUSED;
#endif
    owner=Kh02LifecycleCurrent(p->life);
    if(owner && Kh02ReceiptOwnerHistory(owner,history,count)!=KH02_MEMBER_OK)return KH02_BANK_REFUSED;
#ifdef KH02_FRESH_SETUP_SELFTEST
    if(history_test_fault==2)return KH02_BANK_REFUSED;
#endif
    if(count)memcpy(branch->history,history,count*sizeof(*history));branch->history_count=count;
    return p->closed || p->life->closed || branch->closed?KH02_BANK_CLOSED:KH02_BANK_OK;
}
/* Caller provides an owned authenticated full history. The short ingress lock
   spans only prefix checks and bounded memory copies. No originals/I/O/waits.
   Existing callbacks cause BUSY without changes; subsequent callbacks cannot
   observe consumers halfway through the coordinated update. */
MEMBER_API uint32_t Kh02HistoryRouterAppend(Kh02SaveRouter *p,Kh02DiskLookup *lookup,
    const Kh02ReceiptScope *scope,const Kh02ReceiptIdentity *history,uint32_t count) {
    uint32_t status;if(!p)return KH02_BANK_REFUSED;
    if(GetCurrentThreadId()!=p->thread){Kh02SaveRouterStop(p);return KH02_BANK_REFUSED;}
    if(!TryAcquireSRWLockExclusive(&p->domain.gate))return KH02_BANK_BUSY;
    if(p->domain.readers){ReleaseSRWLockExclusive(&p->domain.gate);return KH02_BANK_BUSY;}
    if(p->closed || p->domain.closed || p->life->closed)status=KH02_BANK_CLOSED;
    else if(!scope || scope->reserved || !scope->seed ||
       scope->cookie!=p->life->branch->scope.cookie || scope->load_id!=p->life->branch->scope.load_id ||
       scope->team!=p->life->branch->scope.team || scope->slot!=p->life->branch->scope.slot ||
       scope->seed_size!=p->life->branch->scope.seed_size ||
       memcmp(scope->seed,p->life->branch->scope.seed,scope->seed_size))status=KH02_BANK_REFUSED;
    /* A not-yet-admitted bootstrap has a private prepared history. Defer growth
       until admission; never update only the disk branch behind its back. */
    else if(p->life->fresh && !p->life->epoch && count!=p->life->branch->history_count)status=KH02_BANK_BUSY;
    else status=Kh02SessionLookupHistory(lookup,scope,history,count,history_commit,p);
    if(status==KH02_BANK_OK && (p->closed || p->domain.closed || p->life->closed || p->life->branch->closed))status=KH02_BANK_CLOSED;
    ReleaseSRWLockExclusive(&p->domain.gate);
    /* Lookup lock contention occurred before commit. Every other failed update
       stops the graph; never retry a partially updated/revoked owner. */
    if(status!=KH02_BANK_OK && status!=KH02_BANK_BUSY)Kh02SaveRouterStop(p);
    return status;
}
/* Narrow future dispatcher borrow boundary. Effects still require independent
   transport/connection authorization; this function does not grant it. It keeps
   the exact owner alive through reentrant transitions and uncertain completion. */
MEMBER_API uint32_t Kh02HistoryRouterExecute(Kh02SaveRouter *p,const unsigned char *proposal,uint32_t size,
    Kh02ReceiptIdentityEffect effect,void *context) {
    Kh02CallbackPin pin={0};Kh02ReceiptOwner *owner;uint32_t status=KH02_MEMBER_REFUSED;
    if(!p || !effect)return status;
    if(GetCurrentThreadId()!=p->thread){Kh02SaveRouterStop(p);return status;}
    if(!Kh02CallbackEnter(&p->domain,&pin))return status;
    owner=Kh02LifecycleCurrent(p->life);
    Kh02SavePipeline *pipeline=pin.selected;
    if(!p->closed && owner && pipeline && pipeline->config.owner==owner && !pipeline->stopped)
        status=Kh02ReceiptOwnerExecuteIdentity(owner,proposal,size,effect,context);
    Kh02CallbackLeave(&pin);Kh02SaveRouterCollect(p);return status;
}
