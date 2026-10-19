#ifndef KH02_GENERATION_CANDIDATE_H
#define KH02_GENERATION_CANDIDATE_H
#include <stdint.h>
#ifdef _WIN32
#define KH02_GATE_API __declspec(dllexport)
#else
#define KH02_GATE_API
#endif

/* Pure state machine, all access confined to one verified native game thread.
   No process access, hooks, dispatching or readiness discovery in this library. */
typedef struct Kh02Lease {
    uint64_t cookie, generation, observation, state, provider, attributes;
} Kh02Lease;
typedef struct Kh02GenerationGate {
    Kh02Lease current;
    Kh02Lease in_flight;
    uint32_t ready, busy;
} Kh02GenerationGate;
enum { KH02_GATE_OK=1, KH02_GATE_REFUSED=2, KH02_GATE_UNCERTAIN=3 };
KH02_GATE_API uint32_t Kh02GateInit(Kh02GenerationGate*,uint64_t);
KH02_GATE_API uint32_t Kh02GateInvalidate(Kh02GenerationGate*);
KH02_GATE_API uint32_t Kh02GateObserve(Kh02GenerationGate*,uint64_t,uint64_t,uint64_t);
/* Only a future independently verified readiness path may call Confirm.
   Observations/timing alone are NOT readiness proof. */
KH02_GATE_API uint32_t Kh02GateConfirm(Kh02GenerationGate*,const Kh02Lease*);
KH02_GATE_API uint32_t Kh02GateBegin(Kh02GenerationGate*,const Kh02Lease*);
KH02_GATE_API uint32_t Kh02GateFinish(Kh02GenerationGate*,const Kh02Lease*,uint32_t);
#endif
