#ifndef KH02_PIPELINE_BOOTSTRAP_CANDIDATE_H
#define KH02_PIPELINE_BOOTSTRAP_CANDIDATE_H
#include "kh02_save_pipeline_candidate.h"
typedef struct Kh02PipelineBootstrap Kh02PipelineBootstrap;
/* Versioned pointer-free setup for a NEW synthetic no-effect test scope only.
   Payload order: header, seed, prefix, UTF8 absolute journal path, full baseline.
   The host preflights/seals files with game closed. Native Open occurs on the
   game owner thread BEFORE publishing hooks, against that preinitialized DB. */
typedef struct Kh02PipelineBootstrapWire {
    uint32_t magic,version,size,flags;
    uint64_t cookie,load_id;
    uint32_t team,slot,seed_size,prefix_size,path_size,baseline_size,reserved[2];
} Kh02PipelineBootstrapWire;
MEMBER_API uint32_t Kh02PipelineBootstrapValidate(const unsigned char*,uint32_t);
/* Config owner/service must be null: bootstrap creates and owns both. Callbacks
   and code remain borrowed. Input bytes need remain alive only through Create. */
MEMBER_API Kh02PipelineBootstrap *Kh02PipelineBootstrapCreate(const Kh02SavePipelineConfig*,const unsigned char*,uint32_t);
/* Borrowed adapter, kept as a stopped tombstone after Drain for late hooks. */
MEMBER_API Kh02SavePipeline *Kh02PipelineBootstrapAdapter(Kh02PipelineBootstrap*);
MEMBER_API uint32_t Kh02PipelineBootstrapDrain(Kh02PipelineBootstrap*);
/* Offline/final lifetime only: all hooks removed and all external callers joined.
   The controlled live protocol instead retains this owner until process exit. */
MEMBER_API uint32_t Kh02PipelineBootstrapDestroy(Kh02PipelineBootstrap**);
#endif
