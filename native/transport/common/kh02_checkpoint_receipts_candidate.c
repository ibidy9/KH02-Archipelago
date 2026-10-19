#include "kh02_checkpoint_receipts_candidate.h"
#include <string.h>
struct Kh02CheckpointReceipts {
    Kh02CheckpointConfig config;DWORD thread;volatile LONG busy,closed,flight;
    uint64_t next_id,context;Kh02ReceiptOwner *parent;
    uint32_t known,size,ready;unsigned char inventory[KH02_RESTORE_INVENTORY_SIZE];
    unsigned char receipts[KH02_RECEIPT_MAX_BYTES];Kh02CheckpointEvidence evidence;
};
static uint32_t enter(Kh02CheckpointReceipts *p) {
    InterlockedIncrement(&p->flight);
    if(GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,1,0)) {
        InterlockedExchange(&p->closed,1);p->config.invalidate_current(p->config.current_context);return 0;
    }
    p->ready=0;return 1;
}
static void leave(Kh02CheckpointReceipts *p,uint32_t entered) {
    if(entered)InterlockedExchange(&p->busy,0);
    InterlockedDecrement(&p->flight);
}
Kh02CheckpointReceipts *Kh02CheckpointCreate(const Kh02CheckpointConfig *c) {
    Kh02CheckpointReceipts *p;
    if(!c || !c->base || c->base>0x0000800000000000ULL-0x04f5f000 || !c->read || !c->capture || !c->restore || !c->current || !c->invalidate_current)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->config=*c;p->thread=GetCurrentThreadId();return p;
}
uintptr_t Kh02CheckpointCapture(Kh02CheckpointReceipts *p,uintptr_t context) {
    uint32_t entered,admitted=0,valid=0;uintptr_t result;Kh02ReceiptOwner *owner=NULL;
    Kh02MembershipTicket ticket={0};Kh02ReceiptLease lease={0};Kh02RestoreSnapshot before,after;
    if(!p)return 0;entered=enter(p);
    if(entered) {
        p->known=0;p->parent=NULL;
        if(p->next_id==UINT64_MAX)InterlockedExchange(&p->closed,1);
        else ++p->next_id;
        if(!InterlockedCompareExchange(&p->closed,0,0)) {
            owner=p->config.current(p->config.current_context);
            if(owner && Kh02ReceiptOwnerStageBegin(owner,&ticket)==KH02_MEMBER_OK) {
                admitted=1;
                valid=Kh02RestoreSnapshotRead(p->config.read,p->config.read_context,p->config.base,context,&before)==KH02_RESTORE_OK;
            }
        }
    }
    result=p->config.capture(context); /* Return is deliberately uninterpreted. */
    if(admitted) {
        valid=valid && !InterlockedCompareExchange(&p->closed,0,0) &&
            p->config.current(p->config.current_context)==owner &&
            Kh02RestoreSnapshotRead(p->config.read,p->config.read_context,p->config.base,context,&after)==KH02_RESTORE_OK &&
            !memcmp(before.primary,after.primary,sizeof(before.primary)) &&
            !memcmp(before.primary,after.checkpoint,sizeof(before.primary));
        if(Kh02ReceiptOwnerStageFinish(owner,&ticket,valid,&lease)==KH02_MEMBER_OK) {
            if(Kh02ReceiptOwnerRead(owner,&lease,p->receipts,sizeof(p->receipts),&p->size)==KH02_BANK_OK) {
                p->parent=owner;p->context=context;memcpy(p->inventory,after.checkpoint,sizeof(p->inventory));p->known=1;
            }
            if(Kh02ReceiptOwnerRelease(owner,&lease)!=KH02_BANK_OK){p->known=0;InterlockedExchange(&p->closed,1);}
        }
    }
    leave(p,entered);return result;
}
uintptr_t Kh02CheckpointRestore(Kh02CheckpointReceipts *p,uintptr_t context,uint32_t mode) {
    uint32_t entered,valid=0;uintptr_t result;Kh02ReceiptOwner *owner=NULL;Kh02RestoreSnapshot before,after;
    if(!p)return 0;entered=enter(p);
    if(entered) {
        owner=p->config.current(p->config.current_context);
        valid=!InterlockedCompareExchange(&p->closed,0,0) && p->known && mode==2 && context==p->context &&
            owner && owner==p->parent && Kh02ReceiptOwnerCheck(owner)==KH02_MEMBER_OK &&
            Kh02RestoreSnapshotRead(p->config.read,p->config.read_context,p->config.base,context,&before)==KH02_RESTORE_OK &&
            !memcmp(before.checkpoint,p->inventory,sizeof(p->inventory));
        /* Current membership is invalid even if original restore fails or uses
           another mode. The checkpoint bytes never infer current inventory. */
        if(owner)Kh02ReceiptOwnerInvalidate(owner);
        p->known=0;
    }
    p->config.invalidate_current(p->config.current_context);
    result=p->config.restore(context,mode);
    if(entered && valid && !InterlockedCompareExchange(&p->closed,0,0) &&
       Kh02RestoreSnapshotRead(p->config.read,p->config.read_context,p->config.base,context,&after)==KH02_RESTORE_OK &&
       !memcmp(after.primary,p->inventory,sizeof(p->inventory)) && !memcmp(after.checkpoint,p->inventory,sizeof(p->inventory))) {
        p->evidence.capture_id=p->next_id;p->evidence.context=context;p->evidence.status=1;p->evidence.receipt_size=p->size;
        memcpy(p->evidence.receipts,p->receipts,p->size);p->ready=1;
    }
    leave(p,entered);return result;
}
uint32_t Kh02CheckpointTake(Kh02CheckpointReceipts *p,Kh02CheckpointEvidence *out) {
    if(!p || !out || GetCurrentThreadId()!=p->thread || InterlockedCompareExchange(&p->busy,0,0) ||
       InterlockedCompareExchange(&p->closed,0,0) || !p->ready)return KH02_BANK_REFUSED;
    *out=p->evidence;p->ready=0;return KH02_BANK_OK;
}
void Kh02CheckpointInvalidate(Kh02CheckpointReceipts *p) {
    if(p)InterlockedExchange(&p->closed,1);
}
uint32_t Kh02CheckpointDestroy(Kh02CheckpointReceipts **handle) {
    Kh02CheckpointReceipts *p;if(!handle)return KH02_BANK_REFUSED;p=*handle;if(!p)return KH02_BANK_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BANK_REFUSED;
    InterlockedExchange(&p->closed,1);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BANK_BUSY;
    /* External exclusion/join of all callers is required before final free. */
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BANK_BUSY;*handle=NULL;return KH02_BANK_OK;
}
