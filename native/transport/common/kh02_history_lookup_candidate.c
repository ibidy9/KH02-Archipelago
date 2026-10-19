/* Original exact-proof implementation remains unchanged and compiled here. */
#include "kh02_session_lookup_candidate.c"
#include "kh02_history_lookup_candidate.h"
uint32_t Kh02SessionLookupHistory(Kh02DiskLookup *p,const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,Kh02HistoryCommit commit,void *context) {
    if(!p || p->thread!=GetCurrentThreadId() || !scope || !commit ||
       count>KH02_RECEIPT_MAX_ITEMS || (count && !history))return KH02_BANK_REFUSED;
    if(!TryAcquireSRWLockExclusive(&p->lock))return KH02_BANK_BUSY;
    if(p->closed)return leave(p,KH02_BANK_CLOSED);
    if(scope->reserved || scope->cookie!=p->scope.cookie || scope->load_id!=p->scope.load_id ||
       scope->team!=p->scope.team || scope->slot!=p->scope.slot || !scope->seed ||
       scope->seed_size!=p->scope.seed_size || memcmp(scope->seed,p->seed,p->scope.seed_size) ||
       count<p->history_count || (p->history_count && memcmp(history,p->history,p->history_count*sizeof(*history))))
        return leave(p,KH02_BANK_REFUSED);
    for(uint32_t i=0;i<count;++i)if(history[i].index!=i)return leave(p,KH02_BANK_REFUSED);
    /* No proof changes. Historical membership remains an exact subset of an
       immutable prefix. Availability commits before new membership is possible. */
    if(commit(context,history,p->history_count,count)!=KH02_BANK_OK) {
        InterlockedExchange(&p->closed,1);return leave(p,KH02_BANK_CLOSED);
    }
    if(count)memcpy(p->history,history,count*sizeof(*history));p->history_count=count;
    return leave(p,KH02_BANK_OK);
}
