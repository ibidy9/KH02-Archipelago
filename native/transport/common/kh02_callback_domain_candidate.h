/* Offline candidate. The containing object still requires an external join
   before destruction. Every access to its retireable graph must use this domain.
   No lock is held while caller code, game callbacks, or destructors execute. */
#ifndef KH02_CALLBACK_DOMAIN_CANDIDATE_H
#define KH02_CALLBACK_DOMAIN_CANDIDATE_H
#include <windows.h>
#include <stdint.h>
typedef struct Kh02CallbackDomain {
    SRWLOCK gate;
    void *active;
    uint64_t readers;
    DWORD owner;
    uint32_t closed;
} Kh02CallbackDomain;
/* Stack-local, zero-initialized, noncopyable token. Caller must not share a
   token between threads. Exactly one Leave follows each successful Enter. */
typedef struct Kh02CallbackPin {
    Kh02CallbackDomain *domain;
    void *selected;
} Kh02CallbackPin;
void Kh02CallbackDomainInit(Kh02CallbackDomain *d);
uint32_t Kh02CallbackEnter(Kh02CallbackDomain *d, Kh02CallbackPin *pin);
uint32_t Kh02CallbackLeave(Kh02CallbackPin *pin);
/* Pins the transition and detaches publication in the same critical section.
   alone=false means an older callback may not yet have entered its pipeline:
   caller must revoke adoption and MUST NOT drain that pipeline in transition. */
uint32_t Kh02CallbackTransition(Kh02CallbackDomain *d, Kh02CallbackPin *pin,
                                uint32_t *alone);
uint32_t Kh02CallbackPublish(Kh02CallbackDomain *d, void *active);
/* Zero readers establishes the callback selection boundary. Returns a current
   root which MUST be retained, even if a subsequent Stop detaches it. Caller
   may reclaim only previously detached objects unreachable from this root.
   Owner-thread serialization and no other graph access paths are prerequisites.
   This is NOT permission to destroy the domain or its current root. */
uint32_t Kh02CallbackSnapshot(Kh02CallbackDomain *d, void **keep);
/* Stop itself borrows the graph. Call Leave after revoking the selected root. */
uint32_t Kh02CallbackStop(Kh02CallbackDomain *d, Kh02CallbackPin *pin);
#endif
