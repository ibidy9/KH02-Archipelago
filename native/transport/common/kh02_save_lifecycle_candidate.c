#include "kh02_save_transition_candidate.c"
#include "kh02_lifecycle_candidate.c"

/* Bounded offline transition composition. Pipeline and lifecycle are borrowed;
   caller retains both (including stopped pipeline) until external users join.
   A fresh pipeline for the new owner is a separate preparation step. */
MEMBER_API uintptr_t Kh02SaveLifecycleRestore(Kh02SavePipeline *pipeline,Kh02Lifecycle *lifecycle,
    uintptr_t context,uint32_t mode,uint32_t *drain_status) {
    uintptr_t result;
    if(!pipeline || !lifecycle || !drain_status)return 0;
    *drain_status=KH02_BIND_BUSY;
    if(pipeline->config.owner!=Kh02LifecycleCurrent(lifecycle) || Kh02SavePipelineQuiesce(pipeline)!=KH02_BIND_OK) {
        Kh02SavePipelineStop(pipeline);Kh02LifecycleStop(lifecycle);
    }
    result=Kh02LifecycleRestore(lifecycle,context,mode);
    /* Ordinary drain now invalidates only the retired owner, after checkpoint
       evidence was validated. It must never be redirected to the new owner. */
    *drain_status=Kh02SavePipelineDrain(pipeline);return result;
}

/* Disk transitions must retire save publication too. LifecycleLoad itself
   invalidates the old owner before invoking the original load. Preserve its
   pending-admission probe by quiescing, rather than prematurely draining. */
MEMBER_API unsigned char Kh02SaveLifecycleLoad(Kh02SavePipeline *pipeline,Kh02Lifecycle *lifecycle,
    uintptr_t manager,uint32_t index,uint32_t *drain_status) {
    unsigned char result;
    if(!pipeline || !lifecycle || !drain_status)return 0;
    *drain_status=KH02_BIND_BUSY;
    if(pipeline->config.owner!=Kh02LifecycleCurrent(lifecycle) || Kh02SavePipelineQuiesce(pipeline)!=KH02_BIND_OK) {
        Kh02SavePipelineStop(pipeline);Kh02LifecycleStop(lifecycle);
    }
    result=Kh02LifecycleLoad(lifecycle,manager,index);
    *drain_status=Kh02SavePipelineDrain(pipeline);return result;
}
