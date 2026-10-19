#ifndef KH02_SAVE_PIPELINE_CANDIDATE_H
#define KH02_SAVE_PIPELINE_CANDIDATE_H
#include "kh02_publication_gate_candidate.h"
#include "kh02_save_snapshot_candidate.h"
typedef struct Kh02SavePipeline Kh02SavePipeline;
typedef unsigned char (*Kh02PipelineStage)(uintptr_t,uint32_t);
typedef uintptr_t (*Kh02PipelineRefresh)(uintptr_t,uint32_t);
/* Offline composition: no entry patching, game discovery, load inference or
   AP delivery. Borrowed owner/service and callback code outlive Drain. Scope,
   baseline receipts and full baseline must be independently validated. Prefix
   is a unique per-session ASCII identifier, 1..64 letters/digits/hyphens.
   Creating thread must also own owner and service. At most eight staged records
   and eight publication calls in this bounded experiment; no eviction/rearming. */
typedef struct Kh02SavePipelineConfig {
    uint64_t base;
    Kh02BoundedRead read;
    void *read_context;
    Kh02PipelineStage stage;
    Kh02PipelineRefresh refresh;
    Kh02PublicationOriginal platform;
    Kh02ReceiptOwner *owner;
    Kh02PublicationService *service;
} Kh02SavePipelineConfig;
MEMBER_API Kh02SavePipeline *Kh02SavePipelineCreate(const Kh02SavePipelineConfig*,
    const Kh02ReceiptScope*,const unsigned char*,uint32_t,const char*,uint32_t);
MEMBER_API unsigned char Kh02SavePipelineStage(Kh02SavePipeline*,uintptr_t,uint32_t);
MEMBER_API uintptr_t Kh02SavePipelineRefresh(Kh02SavePipeline*,uintptr_t,uint32_t);
MEMBER_API unsigned char Kh02SavePipelinePlatform(Kh02SavePipeline*,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
/* Any thread with held external lifetime. Matched AP calls stay refused after
   stop; never silently switch them to vanilla pass-through. */
MEMBER_API void Kh02SavePipelineStop(Kh02SavePipeline*);
/* Creating thread, callers joined. BUSY retains pending ownership. Drain keeps
   the stopped adapter alive for late wrapper entries (originals/code retained).
   Destroy requires removed entry hooks AND exclusion/join of ALL late callers. */
MEMBER_API uint32_t Kh02SavePipelineDrain(Kh02SavePipeline*);
MEMBER_API uint32_t Kh02SavePipelineDestroy(Kh02SavePipeline**);
#endif
