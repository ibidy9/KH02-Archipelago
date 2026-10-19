/* Instrument unchanged composition at existing calls. Wrappers preserve every
   argument, return value and original short-circuit ordering. */
#include "kh02_pipeline_diagnostic.h"
#include <string.h>
static Kh02PipelineDiagnostic *sink;
static volatile LONG diagnostic_count,diagnostic_capture;
void Kh02DiagnosticBind(Kh02PipelineDiagnostic *p){sink=p;memset(p,0,sizeof(*p));p->owner_thread=GetCurrentThreadId();}
void Kh02DiagnosticLog(uint32_t tag,uint32_t status,uint32_t line,uint64_t a,uint64_t b) {
    LONG at;Kh02DiagnosticEvent *e;if(!sink)return;
    for(;;){at=InterlockedCompareExchange(&diagnostic_count,0,0);
        if(at>=128){InterlockedExchange((volatile LONG*)&sink->overflow,1);return;}
        if(InterlockedCompareExchange(&diagnostic_count,at+1,at)==at)break;}
    InterlockedExchange((volatile LONG*)&sink->count,at+1);
    if(at>=128){InterlockedExchange((volatile LONG*)&sink->overflow,1);return;}
    e=&sink->events[at];e->tick=GetTickCount64();e->a=a;e->b=b;e->tag=tag;e->status=status;
    e->line=line;e->thread=GetCurrentThreadId();InterlockedExchange((volatile LONG*)&e->published,1);
}
void Kh02DiagnosticFrozen(uint32_t status,const Kh02FrozenSave *f) {
    if(!sink || InterlockedCompareExchange(&diagnostic_capture,1,0))return;
    sink->capture_status=status;
    if(status==KH02_FREEZE_OK && f && f->data && f->length==KH02_FULL_SAVE_SIZE)
        memcpy(sink->frozen,f->data,KH02_FULL_SAVE_SIZE);
    InterlockedExchange((volatile LONG*)&sink->capture_state,2);
}
static uint32_t diag_begin(Kh02ReceiptOwner *o,Kh02MembershipTicket *t,uint32_t line) {
    uint32_t s=Kh02ReceiptOwnerStageBegin(o,t);Kh02DiagnosticLog(1,s,line,0,0);return s;
}
static uint32_t diag_finish(Kh02ReceiptOwner *o,const Kh02MembershipTicket *t,uint32_t ok,Kh02ReceiptLease *l,uint32_t line) {
    uint32_t s=Kh02ReceiptOwnerStageFinish(o,t,ok,l);Kh02DiagnosticLog(2,s,line,ok,l->revision);return s;
}
static uint32_t diag_snapshot(Kh02BoundedRead r,void *ctx,uint64_t base,uint64_t manager,uint32_t index,Kh02SaveSnapshot *out,uint32_t line) {
    uint32_t s=Kh02SaveSnapshotRead(r,ctx,base,manager,index,out);Kh02DiagnosticLog(3,s,line,index,0);return s;
}
#ifndef KH02_SAVE_HEADER_BINDING
static uint32_t diag_put(Kh02ReceiptBindings *b,uint32_t i,Kh02ReceiptOwner *o,const Kh02ReceiptLease *l,const unsigned char *raw,uint32_t size,uint32_t line) {
    uint32_t s=Kh02ReceiptBindingsPut(b,i,o,l,raw,size);Kh02DiagnosticLog(4,s,line,i,l->revision);return s;
}
#endif
static uint32_t diag_refresh(Kh02ReceiptBindings *b,uint32_t i,const unsigned char *before,const unsigned char *after,uint32_t size,uint32_t line) {
    uint32_t s=Kh02ReceiptBindingsRefresh(b,i,before,after,size);Kh02DiagnosticLog(5,s,line,i,0);return s;
}
static uint32_t diag_freeze(Kh02BoundedRead r,void *ctx,uint64_t base,uint64_t descriptor,Kh02FrozenSave *f,uint32_t line) {
    uint32_t s=Kh02FreezeSave(r,ctx,base,descriptor,f);Kh02DiagnosticLog(6,s,line,0,0);Kh02DiagnosticFrozen(s,f);return s;
}
static uint32_t diag_capture(Kh02ReceiptBindings *b,const unsigned char *raw,uint32_t size,Kh02ReceiptBundleLease *l,uint32_t line) {
    uint32_t s=Kh02ReceiptBindingsCapture(b,raw,size,l);Kh02DiagnosticLog(7,s,line,0,0);return s;
}
static Kh02PublicationGate *diag_gate(Kh02PublicationService *s,const unsigned char *raw,uint32_t size,const unsigned char *token,uint32_t length,Kh02PublicationOriginal fn,uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uint32_t line) {
    Kh02PublicationGate *g=Kh02PublicationCreateAttempt(s,raw,size,token,length,fn,a,b,c,d);
    Kh02DiagnosticLog(8,g!=NULL,line,0,0);return g;
}
static uint32_t diag_execute(Kh02PublicationGate *g,uint32_t line) {
    uint32_t s=Kh02PublicationExecute(g);Kh02DiagnosticLog(9,s,line,0,0);return s;
}
static uint32_t diag_invalidate(Kh02ReceiptOwner *o,uint32_t line) {
    uint32_t s=Kh02ReceiptOwnerInvalidate(o);Kh02DiagnosticLog(10,s,line,0,0);return s;
}
#define Kh02ReceiptOwnerStageBegin(...) diag_begin(__VA_ARGS__,__LINE__)
#define Kh02ReceiptOwnerStageFinish(...) diag_finish(__VA_ARGS__,__LINE__)
#define Kh02SaveSnapshotRead(...) diag_snapshot(__VA_ARGS__,__LINE__)
#define Kh02ReceiptBindingsPut(...) diag_put(__VA_ARGS__,__LINE__)
#define Kh02ReceiptBindingsRefresh(...) diag_refresh(__VA_ARGS__,__LINE__)
#define Kh02FreezeSave(...) diag_freeze(__VA_ARGS__,__LINE__)
#define Kh02ReceiptBindingsCapture(...) diag_capture(__VA_ARGS__,__LINE__)
#define Kh02PublicationCreateAttempt(...) diag_gate(__VA_ARGS__,__LINE__)
#define Kh02PublicationExecute(...) diag_execute(__VA_ARGS__,__LINE__)
#define Kh02ReceiptOwnerInvalidate(...) diag_invalidate(__VA_ARGS__,__LINE__)
#ifdef KH02_SAVE_HEADER_BINDING
#include "kh02_save_header_bindings_candidate.h"
static uint32_t diag_put_header(Kh02ReceiptBindings *b,uint32_t i,Kh02ReceiptOwner *o,const Kh02ReceiptLease *l,
    const unsigned char *raw,uint32_t size,const unsigned char *header,uint32_t header_size,uint32_t line) {
    uint32_t s=Kh02ReceiptBindingsPutHeader(b,i,o,l,raw,size,header,header_size);
    Kh02DiagnosticLog(4,s,line,i,l->revision);return s;
}
#define Kh02ReceiptBindingsPutHeader(...) diag_put_header(__VA_ARGS__,__LINE__)
#ifdef KH02_REUSABLE_SAVE_PIPELINE
#include "kh02_reusable_save_pipeline_candidate.c"
#else
#include "kh02_save_header_pipeline_candidate.c"
#endif
#undef Kh02ReceiptBindingsPutHeader
#else
#include "kh02_save_pipeline_candidate.c"
#endif


#undef Kh02ReceiptOwnerStageBegin
#undef Kh02ReceiptOwnerStageFinish
#undef Kh02SaveSnapshotRead
#undef Kh02ReceiptBindingsPut
#undef Kh02ReceiptBindingsRefresh
#undef Kh02FreezeSave
#undef Kh02ReceiptBindingsCapture
#undef Kh02PublicationCreateAttempt
#undef Kh02PublicationExecute
#undef Kh02ReceiptOwnerInvalidate
