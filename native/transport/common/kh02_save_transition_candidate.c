/* New compatibility boundary alongside the proven save pipeline. Its ordinary
   stop/drain semantics remain compiled unchanged below. */
#ifdef KH02_SAVE_TRANSPORT_DIAGNOSTIC
#include "kh02_save_transport_diagnostic_pipeline.c"
#else
#include "kh02_save_pipeline_candidate.c"
#endif

/* Owner thread only. Prevent new matched publication while preserving the
   current receipt owner long enough for checkpoint validation. This is NOT
   cleanup, permission to adopt, or a guarantee that an admitted save was undone.
   BUSY/refusal requires the lifecycle caller to revoke adoption, run the original
   transition, and retain this pipeline. Never wait inside the game callback. */
MEMBER_API uint32_t Kh02SavePipelineQuiesce(Kh02SavePipeline *p) {
    Kh02PublicationGate *gate;
    if(!p || GetCurrentThreadId()!=p->thread || p->drained)return KH02_BIND_REFUSED;
    InterlockedExchange(&p->stopped,1);
    for(uint32_t i=0;i<8;++i) {
        gate=InterlockedCompareExchangePointer((void*volatile*)&p->attempts[i].gate,NULL,NULL);
        if(gate)Kh02PublicationRevoke(gate);
    }
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BIND_BUSY;
    return Kh02ReceiptOwnerCheck(p->config.owner)==KH02_MEMBER_OK?KH02_BIND_OK:KH02_BIND_REFUSED;
}
