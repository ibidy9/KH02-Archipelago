#ifndef KH02_ACTION_OBSERVER_V32_H
#define KH02_ACTION_OBSERVER_V32_H
#include <stdint.h>
/* Separate lifetime counters from epoch-local identities. This ABI is read-only. */
enum { KH02_OBS_ACCEPTED, KH02_OBS_THREAD, KH02_OBS_INACTIVE, KH02_OBS_FAULT,
       KH02_OBS_OWNER, KH02_OBS_EPOCH, KH02_OBS_SESSION, KH02_OBS_KIND,
       KH02_OBS_RECEIVER, KH02_OBS_TABLE, KH02_OBS_CHAIN, KH02_OBS_UNSTABLE,
       KH02_OBS_REASONS };
typedef struct Kh02AbilityDiagnosticCounters {
    uint64_t raw,reasons[KH02_OBS_REASONS],target_attempts[4];
} Kh02AbilityDiagnosticCounters;
typedef struct Kh02AbilityDiagnosticSample {
    uint64_t receiver,vtable,actor,provider,caller;
    uint32_t kind;
    int32_t reason;
    uint64_t manager,action_state,state_vtable;
    uint32_t state_kind,substate;
} Kh02AbilityDiagnosticSample;
typedef struct Kh02DefaultAbilitySnapshot {
    uint64_t epoch,overflow,coherent;
    Kh02AbilityDiagnosticCounters hooks[4];
    uint64_t epoch_counts[4][4];
    Kh02AbilityDiagnosticSample samples[4][4];
    uint64_t callers[4][4][4];
} Kh02DefaultAbilitySnapshot;
#endif
