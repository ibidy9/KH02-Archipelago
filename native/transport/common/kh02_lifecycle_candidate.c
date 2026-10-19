/* Side-by-side composition: retain the unchanged component implementations and
   access their private owned storage only inside this compilation unit. */
#include "kh02_disk_branch_candidate.c"
#include "kh02_checkpoint_receipts_candidate.c"
#include "kh02_lifecycle_candidate.h"
#ifdef KH02_FRESH_LIFECYCLE
#include "kh02_fresh_entry_candidate.h"
#endif
struct Kh02Lifecycle {
    Kh02DiskBranch *branch;Kh02CheckpointConfig checkpoint_config;Kh02DiskLoadOriginal original;
    Kh02CheckpointReceipts *checkpoint,*retained[64];uint32_t count;uint64_t epoch;
    DWORD thread;volatile LONG busy,flight,closed;Kh02CheckpointEvidence evidence;
#ifdef KH02_FRESH_LIFECYCLE
    Kh02FreshEntry *fresh;Kh02FreshCopy fresh_original;uint32_t fresh_lineage;
#endif
};
static void lifecycle_revoke(void *opaque){Kh02Lifecycle *p=opaque;
#ifdef KH02_FRESH_LIFECYCLE
    Kh02FreshRevoke(p->fresh);
#endif
    invalidate(p->branch);}
static Kh02ReceiptOwner *lifecycle_current(void *opaque) {
    Kh02Lifecycle *p=opaque;
    if(InterlockedCompareExchange(&p->closed,0,0))return NULL;
#ifdef KH02_FRESH_LIFECYCLE
    if(p->fresh_lineage) {
        Kh02ReceiptOwner *owner=InterlockedCompareExchangePointer(&p->branch->active,NULL,NULL);
        return owner && Kh02ReceiptOwnerCheck(owner)==KH02_MEMBER_OK?owner:NULL;
    }
#endif
    return Kh02DiskBranchCurrent(p->branch);
}
static uint32_t lifecycle_enter(Kh02Lifecycle *p) {
    InterlockedIncrement(&p->flight);
    if(GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,1,0)) {
        InterlockedExchange(&p->closed,1);lifecycle_revoke(p);return 0;
    }
    return 1;
}
static void lifecycle_leave(Kh02Lifecycle *p,uint32_t entered) {
    if(entered)InterlockedExchange(&p->busy,0);InterlockedDecrement(&p->flight);
}
static uint32_t advance(Kh02Lifecycle *p) {
    if(p->epoch==UINT64_MAX){InterlockedExchange(&p->closed,1);lifecycle_revoke(p);return 0;}
    ++p->epoch;return !InterlockedCompareExchange(&p->closed,0,0);
}
static void require_quiescent(Kh02Lifecycle *p) {
    Kh02ReceiptOwner *owner=lifecycle_current(p);Kh02MembershipTicket ticket={0};Kh02ReceiptLease lease={0};
    if(!owner)return;
    /* No game call occurs in this probe. A pending effect/stage must close
       adoption, rather than allowing its later completion to affect a new owner. */
    if(Kh02ReceiptOwnerStageBegin(owner,&ticket)!=KH02_MEMBER_OK)goto refused;
    if(Kh02ReceiptOwnerStageFinish(owner,&ticket,1,&lease)!=KH02_MEMBER_OK)goto refused;
    if(Kh02ReceiptOwnerRelease(owner,&lease)!=KH02_BANK_OK)goto refused;
    return;
refused:InterlockedExchange(&p->closed,1);lifecycle_revoke(p);
}
static Kh02ReceiptOwner *adopt(Kh02Lifecycle *p,unsigned char *receipts,uint32_t size) {
    Kh02DiskBranch *b=p->branch;Kh02ReceiptOwner *owner;Kh02ReceiptScope scope=b->scope;
    if(b->count==64 || size<56 || InterlockedCompareExchange(&p->closed,0,0))return NULL;
    scope.load_id=p->epoch;memcpy(receipts+24,&scope.load_id,8);
    owner=Kh02ReceiptOwnerCreate(&scope,b->history,b->history_count,b->supported,b->supported_count,receipts,size);
    if(!owner)return NULL;
    b->retained[b->count++]=owner;InterlockedExchangePointer(&b->active,owner);
    if(InterlockedCompareExchange(&p->closed,0,0)){lifecycle_revoke(p);return NULL;}
    return owner;
}
Kh02Lifecycle *Kh02LifecycleCreate(const Kh02DiskLoadConfig *disk,const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count,const Kh02CheckpointConfig *checkpoint) {
    Kh02Lifecycle *p;
    if(!disk || !checkpoint || checkpoint->base!=disk->base || !checkpoint->capture || !checkpoint->restore)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->branch=Kh02DiskBranchCreate(disk,scope,history,count,ids,id_count);
    if(!p->branch){VirtualFree(p,0,MEM_RELEASE);return NULL;}
    p->thread=GetCurrentThreadId();p->original=disk->original;p->checkpoint_config=*checkpoint;
    p->checkpoint_config.read=disk->read;p->checkpoint_config.read_context=disk->read_context;
    p->checkpoint_config.current=lifecycle_current;p->checkpoint_config.current_context=p;
    p->checkpoint_config.invalidate_current=lifecycle_revoke;return p;
}
unsigned char Kh02LifecycleLoad(Kh02Lifecycle *p,uintptr_t manager,uint32_t index) {
    uint32_t entered,eligible=0;unsigned char result;Kh02DiskBranch *b;
    if(!p)return 0;entered=lifecycle_enter(p);b=p->branch;
    if(entered)require_quiescent(p);lifecycle_revoke(p);
    if(entered) {
#ifdef KH02_FRESH_LIFECYCLE
        p->fresh_lineage=0;
#endif
        if(p->checkpoint)Kh02CheckpointInvalidate(p->checkpoint);p->checkpoint=NULL;
        eligible=advance(p);
        result=Kh02DiskLoadInvoke(b->load,manager,index,&b->result);
        b->generation=b->result.generation;
        if(eligible && !InterlockedCompareExchange(&p->closed,0,0)) {
            if(b->result.status==KH02_LOAD_KNOWN && Kh02DiskLoadCheck(b->load,b->generation) &&
               !adopt(p,b->result.receipts,b->result.receipt_size))InterlockedExchange(&p->closed,1);
            if(p->count<64)p->checkpoint=Kh02CheckpointCreate(&p->checkpoint_config);
            if(p->checkpoint)p->retained[p->count++]=p->checkpoint;
            else {InterlockedExchange(&p->closed,1);lifecycle_revoke(p);}
        }
    } else result=p->original(manager,index);
    lifecycle_leave(p,entered);return result;
}
uintptr_t Kh02LifecycleCapture(Kh02Lifecycle *p,uintptr_t context) {
    uint32_t entered;uintptr_t result;if(!p)return 0;entered=lifecycle_enter(p);
    result=entered && p->checkpoint && !InterlockedCompareExchange(&p->closed,0,0)?
        Kh02CheckpointCapture(p->checkpoint,context):p->checkpoint_config.capture(context);
    lifecycle_leave(p,entered);return result;
}
uintptr_t Kh02LifecycleRestore(Kh02Lifecycle *p,uintptr_t context,uint32_t mode) {
    uint32_t entered,eligible=0;uintptr_t result;Kh02ReceiptOwner *owner;
    if(!p)return 0;entered=lifecycle_enter(p);if(entered){require_quiescent(p);eligible=advance(p);}
    if(entered && eligible && p->checkpoint) {
        result=Kh02CheckpointRestore(p->checkpoint,context,mode);
        if(!InterlockedCompareExchange(&p->closed,0,0) && Kh02CheckpointTake(p->checkpoint,&p->evidence)==KH02_BANK_OK) {
            owner=adopt(p,p->evidence.receipts,p->evidence.receipt_size);
            if(owner) {
                /* Exact original checkpoint lineage survives Retry. Rebind its
                   retained set to the newly validated owner, without pretending
                   another game capture happened or advancing capture_id. */
                p->checkpoint->parent=owner;p->checkpoint->size=p->evidence.receipt_size;
                memcpy(p->checkpoint->receipts,p->evidence.receipts,p->evidence.receipt_size);p->checkpoint->known=1;
            } else {InterlockedExchange(&p->closed,1);lifecycle_revoke(p);}
        }
    } else {lifecycle_revoke(p);result=p->checkpoint_config.restore(context,mode);}
    lifecycle_leave(p,entered);return result;
}
Kh02ReceiptOwner *Kh02LifecycleCurrent(Kh02Lifecycle *p) {
    return p && GetCurrentThreadId()==p->thread && !InterlockedCompareExchange(&p->busy,0,0)?lifecycle_current(p):NULL;
}
uint64_t Kh02LifecycleEpoch(Kh02Lifecycle *p){return p && GetCurrentThreadId()==p->thread?p->epoch:0;}
uint32_t Kh02LifecycleStop(Kh02Lifecycle *p) {
    if(!p)return KH02_BANK_REFUSED;
    InterlockedExchange(&p->closed,1);lifecycle_revoke(p);
    return InterlockedCompareExchange(&p->flight,0,0)?KH02_BANK_BUSY:KH02_BANK_OK;
}
uint32_t Kh02LifecycleDestroy(Kh02Lifecycle **handle) {
    Kh02Lifecycle *p;if(!handle)return KH02_BANK_REFUSED;p=*handle;if(!p)return KH02_BANK_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BANK_REFUSED;
    if(Kh02LifecycleStop(p)!=KH02_BANK_OK)return KH02_BANK_BUSY;
    for(uint32_t i=0;i<p->count;++i)if(Kh02CheckpointDestroy(&p->retained[i])!=KH02_BANK_OK)return KH02_BANK_BUSY;
#ifdef KH02_FRESH_LIFECYCLE
    if(Kh02FreshDestroy(&p->fresh)!=KH02_BANK_OK)return KH02_BANK_BUSY;
#endif
    if(Kh02DiskBranchDestroy(&p->branch)!=KH02_BANK_OK)return KH02_BANK_BUSY;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BANK_BUSY;*handle=NULL;return KH02_BANK_OK;
}
#ifdef KH02_FRESH_LIFECYCLE
/* Additive offline composition. No native hook or public packet accepts these
   trusted bootstrap inputs. Legacy builds compile no fresh-entry lane. */
KH02_RECEIPT_API uint32_t Kh02LifecycleArmFresh(Kh02Lifecycle *p,const Kh02FreshConfig *config,
    const unsigned char *empty,uint32_t size) {
    unsigned char rebound[4152];Kh02ReceiptScope scope;
    if(!p || !config || !empty || size<56 || size>sizeof(rebound) || GetCurrentThreadId()!=p->thread ||
       p->fresh || p->epoch || p->count || p->branch->generation || p->branch->active ||
       InterlockedCompareExchange(&p->closed,0,0) || InterlockedCompareExchange(&p->busy,0,0) ||
       config->base!=p->checkpoint_config.base || config->read!=p->checkpoint_config.read ||
       config->read_context!=p->checkpoint_config.read_context)return KH02_BANK_REFUSED;
    scope=p->branch->scope;scope.load_id=1;
    memcpy(rebound,empty,size);memcpy(rebound+24,&scope.load_id,8);
    p->fresh=Kh02FreshCreate(config,&scope,p->branch->history,p->branch->history_count,
        p->branch->supported,p->branch->supported_count,rebound,size);
    if(!p->fresh)return KH02_BANK_REFUSED;
    p->fresh_original=config->original;return KH02_BANK_OK;
}
KH02_RECEIPT_API uintptr_t Kh02LifecycleFreshCopy(Kh02Lifecycle *p,uintptr_t caller,uintptr_t destination,uintptr_t source) {
    uintptr_t result;uint32_t entered;Kh02ReceiptOwner *owner;
    if(!p || !p->fresh_original)return 0;
    /* Nested copies inside the proven checkpoint wrapper are not new entries. */
    if(caller!=p->checkpoint_config.base+0x56e280 && caller!=p->checkpoint_config.base+0x56e277)
        return p->fresh_original(destination,source);
    entered=lifecycle_enter(p);
    if(!entered || p->epoch || p->checkpoint || !advance(p))lifecycle_revoke(p);
    result=Kh02FreshCopyInvoke(p->fresh,caller,destination,source);
    owner=entered && !InterlockedCompareExchange(&p->closed,0,0)?Kh02FreshCurrent(p->fresh):NULL;
    if(owner) {
        p->fresh_lineage=1;InterlockedExchangePointer(&p->branch->active,owner);
        p->checkpoint=Kh02CheckpointCreate(&p->checkpoint_config);
        if(p->checkpoint)p->retained[p->count++]=p->checkpoint;
        else {InterlockedExchange(&p->closed,1);lifecycle_revoke(p);}
    }
    lifecycle_leave(p,entered);return result;
}
#endif
