#ifndef KH02_PIPELINE_DIAGNOSTIC_H
#define KH02_PIPELINE_DIAGNOSTIC_H
#include "kh02_pipeline_transport_candidate.h"
typedef struct Kh02DiagnosticEvent {
    uint64_t tick,a,b;
    uint32_t tag,status,line,thread,published,reserved;
} Kh02DiagnosticEvent;
typedef struct Kh02PipelineDiagnostic {
    int32_t count,overflow,capture_state;
    uint32_t capture_status,owner_thread,reserved;
    Kh02DiagnosticEvent events[128];
    unsigned char frozen[KH02_FULL_SAVE_SIZE];
} Kh02PipelineDiagnostic;
typedef struct Kh02DiagnosticBlock {
    Kh02RouteBlock transport;
    uint32_t setup_size,reserved_setup;
    unsigned char setup[KH02_PIPELINE_SETUP_MAX];
    int32_t count,overflow;
    Kh02PipelineEvent events[8];
    Kh02PipelineDiagnostic diagnostic;
} Kh02DiagnosticBlock;
void Kh02DiagnosticBind(Kh02PipelineDiagnostic*);
void Kh02DiagnosticLog(uint32_t,uint32_t,uint32_t,uint64_t,uint64_t);
void Kh02DiagnosticFrozen(uint32_t,const Kh02FrozenSave*);
#endif
