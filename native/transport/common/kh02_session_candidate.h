#ifndef KH02_SESSION_CANDIDATE_H
#define KH02_SESSION_CANDIDATE_H
#include "kh02_generation_candidate.h"
#include "kh02_combined_route_candidate.h"

/* Offline orchestration only. Samples are supplied by the test harness, not
   trusted process memory. No hook, process reader, or game effect is exported. */
typedef struct Kh02SessionSample {
    uint64_t generation, context, world, state, provider, attributes, pending_map;
    uint32_t guards_verified, phase, title_flags, travel_flags, world_flag;
    uint32_t reserved;
} Kh02SessionSample;
typedef struct Kh02SessionCandidate {
    Kh02GenerationGate gate;
    uint64_t context, world, last_sequence;
    uint32_t stopped, uncertain;
} Kh02SessionCandidate;
typedef uint32_t (*Kh02SimulatedEffect)(void *);

KH02_GATE_API uint32_t Kh02SessionInit(Kh02SessionCandidate*, uint64_t);
KH02_GATE_API uint32_t Kh02SessionObserve(Kh02SessionCandidate*, const Kh02SessionSample*);
KH02_GATE_API uint32_t Kh02SessionTravel(Kh02SessionCandidate*, const Kh02RoutePolicy*,
    const Kh02RouteRequest*, Kh02RouteResult*);
KH02_GATE_API uint32_t Kh02SessionExecute(Kh02SessionCandidate*, const Kh02Lease*, uint64_t,
    const Kh02SessionSample*, Kh02SimulatedEffect, void*);
KH02_GATE_API uint32_t Kh02SessionStop(Kh02SessionCandidate*);
#endif
