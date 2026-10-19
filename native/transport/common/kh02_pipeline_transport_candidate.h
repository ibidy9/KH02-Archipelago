#ifndef KH02_PIPELINE_TRANSPORT_CANDIDATE_H
#define KH02_PIPELINE_TRANSPORT_CANDIDATE_H
#include "kh02_combined_route_candidate.h"
#include "kh02_pipeline_bootstrap_candidate.h"
#define KH02_PIPELINE_SETUP_MAX (64u+55u+32u+1023u+KH02_FULL_SAVE_SIZE)
#define KH02_PIPELINE_MESSAGE 0x826eu
#define KH02_PIPELINE_RETAINED 8u
typedef struct Kh02PipelineEvent {
    uint64_t started,finished;
    uint32_t thread,result,published,reserved;
    unsigned char frozen[KH02_FULL_SAVE_SIZE];
} Kh02PipelineEvent;
typedef struct Kh02PipelineBlock {
    Kh02RouteBlock transport;
    uint32_t setup_size,reserved_setup;
    unsigned char setup[KH02_PIPELINE_SETUP_MAX];
    int32_t count,overflow;
    Kh02PipelineEvent events[8];
} Kh02PipelineBlock;
/* Candidate ABI only. Native message handler/live release not implemented here.
   A retained acknowledgment must have active=0, entry_restored=0, in_flight=0;
   the controller must not release ownership until original-process exit. */
#endif
