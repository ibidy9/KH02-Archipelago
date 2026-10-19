#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "kh02_save_pipeline_candidate.h"
typedef struct SaveAttempt {
    Kh02FrozenSave frozen;
    Kh02ReceiptBundleLease bundle;
    Kh02PublicationGate *volatile gate;
} SaveAttempt;
struct Kh02SavePipeline {
    Kh02SavePipelineConfig config;
    DWORD thread;
    volatile LONG stopped,flight,stages,publications,associated;
    uint32_t drained;
    char prefix[65];
    Kh02ReceiptBindings *bindings;
    Kh02ReceiptLease stages_held[8];
    SaveAttempt attempts[8];
};
static LONG stopped(Kh02SavePipeline *p) {return InterlockedCompareExchange(&p->stopped,0,0);}
void Kh02SavePipelineStop(Kh02SavePipeline *p) {
    uint32_t i;Kh02PublicationGate *gate;
    if(!p)return;
    InterlockedExchange(&p->stopped,1);
    /* owner is borrowed and remains held through Drain, not after it. */
    if(!p->drained)Kh02ReceiptOwnerInvalidate(p->config.owner);
    for(i=0;i<8;i++) {
        gate=InterlockedCompareExchangePointer((void*volatile*)&p->attempts[i].gate,NULL,NULL);
        if(gate)Kh02PublicationRevoke(gate);
    }
}
Kh02SavePipeline *Kh02SavePipelineCreate(const Kh02SavePipelineConfig *config,
    const Kh02ReceiptScope *scope,const unsigned char *raw,uint32_t size,const char *prefix,uint32_t length) {
    Kh02SavePipeline *p;uint32_t i;
    if(!config || !config->read || !config->stage || !config->refresh || !config->platform ||
       !config->owner || !config->service || !config->base || config->base>0x0000800000000000ULL-0x04f5f000 ||
       !prefix || !length || length>64 || Kh02ReceiptOwnerCheck(config->owner)!=KH02_MEMBER_OK)return NULL;
    for(i=0;i<length;i++)if(!((prefix[i]>='a'&&prefix[i]<='z') || (prefix[i]>='A'&&prefix[i]<='Z') ||
        (prefix[i]>='0'&&prefix[i]<='9') || prefix[i]=='-'))return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->config=*config;p->thread=GetCurrentThreadId();memcpy(p->prefix,prefix,length);
    p->bindings=Kh02ReceiptBindingsCreate(scope,raw,size);
    if(!p->bindings){VirtualFree(p,0,MEM_RELEASE);return NULL;}
    return p;
}
unsigned char Kh02SavePipelineStage(Kh02SavePipeline *p,uintptr_t manager,uint32_t index) {
    Kh02MembershipTicket ticket={0};Kh02SaveSnapshot snapshot;unsigned char result;
    uint32_t admitted=0,success;LONG at=-1;
    if(!p)return 0;
    InterlockedIncrement(&p->flight);
    if(manager==p->config.base+0x4485500 && !stopped(p)) {
        at=InterlockedIncrement(&p->stages)-1;
        if(at>=8 || GetCurrentThreadId()!=p->thread ||
           Kh02ReceiptOwnerStageBegin(p->config.owner,&ticket)!=KH02_MEMBER_OK)Kh02SavePipelineStop(p);
        else admitted=1;
    }
    /* Original always runs, even for failed admission/wrong-thread/reentrancy. */
    result=p->config.stage(manager,index);
    if(admitted) {
        success=result && !stopped(p) && Kh02SaveSnapshotRead(p->config.read,p->config.read_context,
            p->config.base,manager,index,&snapshot)==KH02_SNAPSHOT_OK;
        if(Kh02ReceiptOwnerStageFinish(p->config.owner,&ticket,success,&p->stages_held[at])!=KH02_MEMBER_OK ||
           !success || Kh02ReceiptBindingsPut(p->bindings,index,p->config.owner,&p->stages_held[at],
                snapshot.record,KH02_SAVE_RECORD_SIZE)!=KH02_BIND_OK)Kh02SavePipelineStop(p);
        else InterlockedExchange(&p->associated,1);
        if(p->stages_held[at].serial)Kh02ReceiptOwnerRelease(p->config.owner,&p->stages_held[at]);
    }
    InterlockedDecrement(&p->flight);return result;
}
uintptr_t Kh02SavePipelineRefresh(Kh02SavePipeline *p,uintptr_t manager,uint32_t index) {
    Kh02MembershipTicket ticket={0};Kh02SaveSnapshot before,after;Kh02ReceiptLease lease={0};
    uint32_t admitted=0,success=0;uintptr_t result;LONG at=-1;
    if(!p)return 0;
    InterlockedIncrement(&p->flight);
    if(manager==p->config.base+0x4485500 && !stopped(p)) {
        /* Refresh needs the same exclusion as stage, but preserves membership. */
        at=InterlockedIncrement(&p->stages)-1;
        if(at>=8 || GetCurrentThreadId()!=p->thread || Kh02ReceiptOwnerStageBegin(p->config.owner,&ticket)!=KH02_MEMBER_OK)
            Kh02SavePipelineStop(p);
        else {admitted=1;success=Kh02SaveSnapshotRead(p->config.read,p->config.read_context,p->config.base,manager,index,&before)==KH02_SNAPSHOT_OK;}
    }
    result=p->config.refresh(manager,index);
    if(admitted) {
        success=success && !stopped(p) && Kh02SaveSnapshotRead(p->config.read,p->config.read_context,p->config.base,manager,index,&after)==KH02_SNAPSHOT_OK;
        /* The cleanup slot was reserved before admitting a returned lease. */
        if(Kh02ReceiptOwnerStageFinish(p->config.owner,&ticket,success,&lease)!=KH02_MEMBER_OK || !success ||
           Kh02ReceiptBindingsRefresh(p->bindings,index,before.record,after.record,KH02_SAVE_RECORD_SIZE)!=KH02_BIND_OK)
            Kh02SavePipelineStop(p);
        if(lease.serial && Kh02ReceiptOwnerRelease(p->config.owner,&lease)!=KH02_BANK_OK) {
            p->stages_held[at]=lease;
            Kh02SavePipelineStop(p);
        }
    }
    InterlockedDecrement(&p->flight);return result;
}
static unsigned char publish_original(uintptr_t opaque,uintptr_t arguments,uintptr_t unused1,uintptr_t unused2,uintptr_t descriptor) {
    Kh02SavePipeline *p=(Kh02SavePipeline*)opaque;uintptr_t *args=(uintptr_t*)arguments;
    (void)unused1;(void)unused2;
    if(stopped(p))return 0;
    return p->config.platform(args[0],args[1],args[2],args[3],descriptor);
}
unsigned char Kh02SavePipelinePlatform(Kh02SavePipeline *p,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t caller) {
    unsigned char result=0;LONG at;SaveAttempt *attempt;Kh02PublicationGate *gate;uint32_t status;
    uintptr_t args[4]={a,b,c,d};char token[96];
    if(!p)return 0;
    InterlockedIncrement(&p->flight);
    if(caller!=p->config.base+0x53ee44) {
        result=p->config.platform(a,b,c,d,e);goto done;
    }
    if(stopped(p))goto done;
    if(a!=p->config.base+0x4485688 || e!=p->config.base+0x4485520){Kh02SavePipelineStop(p);goto done;}
    at=InterlockedIncrement(&p->publications)-1;
    if(at>=8 || !InterlockedCompareExchange(&p->associated,0,0)){Kh02SavePipelineStop(p);goto done;}
    attempt=&p->attempts[at];
    if(Kh02FreezeSave(p->config.read,p->config.read_context,p->config.base,e,&attempt->frozen)!=KH02_FREEZE_OK ||
       Kh02ReceiptBindingsCapture(p->bindings,attempt->frozen.data,attempt->frozen.length,&attempt->bundle)!=KH02_BIND_OK) {
        Kh02SavePipelineStop(p);goto done;
    }
    snprintf(token,sizeof(token),"%s-%u",p->prefix,(unsigned)at+1);
    gate=Kh02PublicationCreateAttempt(p->config.service,(const unsigned char*)attempt->bundle.data,
        attempt->bundle.data->size,(const unsigned char*)token,(uint32_t)strlen(token),publish_original,
        (uintptr_t)p,(uintptr_t)args,0,0);
    if(!gate){Kh02SavePipelineStop(p);goto done;}
    InterlockedExchangePointer((void*volatile*)&attempt->gate,gate);
    if(stopped(p))Kh02PublicationRevoke(gate);
    /* Independent canonical allocation now owns every byte through original. */
    Kh02ReceiptBindingsRelease(p->bindings,&attempt->bundle);
    Kh02ReleaseFrozenSave(&attempt->frozen);
    status=Kh02PublicationExecute(gate);
    result=status==KH02_PUBLICATION_SAVED;
    if(!result)Kh02SavePipelineStop(p);
done:
    InterlockedDecrement(&p->flight);return result;
}
uint32_t Kh02SavePipelineDrain(Kh02SavePipeline *p) {
    uint32_t i;
    if(!p)return KH02_BIND_REFUSED;
    if(GetCurrentThreadId()!=p->thread)return KH02_BIND_REFUSED;
    if(p->drained)return KH02_BIND_OK;
    Kh02SavePipelineStop(p);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BIND_BUSY;
    for(i=0;i<8;i++) {
        SaveAttempt *attempt=&p->attempts[i];
        if(p->stages_held[i].serial && Kh02ReceiptOwnerRelease(p->config.owner,&p->stages_held[i])!=KH02_BANK_OK)return KH02_BIND_BUSY;
        if(attempt->bundle.data && Kh02ReceiptBindingsRelease(p->bindings,&attempt->bundle)!=KH02_BIND_OK)return KH02_BIND_BUSY;
        if(Kh02ReleaseFrozenSave(&attempt->frozen)!=KH02_FREEZE_OK)return KH02_BIND_BUSY;
        if(Kh02PublicationClose((Kh02PublicationGate**)&attempt->gate)!=KH02_JOURNAL_DURABLE)return KH02_BIND_BUSY;
    }
    if(Kh02ReceiptBindingsDestroy(&p->bindings)!=KH02_BIND_OK)return KH02_BIND_BUSY;
    p->drained=1;return KH02_BIND_OK;
}
uint32_t Kh02SavePipelineDestroy(Kh02SavePipeline **owner) {
    Kh02SavePipeline *p;uint32_t status;
    if(!owner)return KH02_BIND_REFUSED;p=*owner;if(!p)return KH02_BIND_OK;
    status=Kh02SavePipelineDrain(p);if(status!=KH02_BIND_OK)return status;
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BIND_BUSY;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BIND_REFUSED;
    *owner=NULL;return KH02_BIND_OK;
}
