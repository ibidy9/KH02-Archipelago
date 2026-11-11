#ifndef KH02_EXPERIMENT_REFLECTED_GRANT_H
#define KH02_EXPERIMENT_REFLECTED_GRANT_H
#include "kh02_receipt_grant_candidate.h"
/* Experimental synchronous boundary. No UE4SS imports, hooks or game discovery.
   Check must verify actual thread, current AP owner/context and exact reflected
   function contracts. Invoke completes on that thread before returning. */
typedef uint32_t (*Kh02ReflectedCheck)(void *, uint64_t *);
typedef uint32_t (*Kh02ReflectedInvoke)(void *, uint32_t, void *, uint32_t);

typedef struct Kh02ReflectedConvertParams {
    uint64_t world_context;
    uint8_t battle_id, reserved[7];
    Kh02ReceiptItemName result;
} Kh02ReflectedConvertParams;

typedef struct Kh02ReflectedGrantParams {
    uint64_t world_context;
    Kh02ReceiptItemName name;
    uint8_t show, reserved[7];
} Kh02ReflectedGrantParams;

typedef struct Kh02ReflectedGrantAdapter {
    Kh02ReceiptGrant delegate;
    Kh02ReflectedCheck check;
    Kh02ReflectedInvoke invoke;
    void *bridge;
    uint64_t world_context;
    uint32_t fault;
} Kh02ReflectedGrantAdapter;

/* Operations: 1=converter, 2=grant. Attach changes only a caller-owned packet.
   Adapter must outlive that synchronous operation; never persist it across load. */
#ifdef __cplusplus
extern "C" {
#endif
KH02_RECEIPT_API uint32_t Kh02ReflectedGrantAttach(Kh02ReceiptGrant *, Kh02ReflectedGrantAdapter *,
                                                   Kh02ReflectedCheck, Kh02ReflectedInvoke, void *);
#ifdef __cplusplus
}
#endif
#endif
