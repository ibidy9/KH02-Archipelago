#ifndef KH02_FRESH_ENTRY_CANDIDATE_H
#define KH02_FRESH_ENTRY_CANDIDATE_H
#include "kh02_receipt_owner_candidate.h"
#include "kh02_restore_snapshot_candidate.h"
typedef struct Kh02FreshEntry Kh02FreshEntry;
/* Private bootstrap authority, NOT a caller packet or mere empty-directory
   flag. Future live composition must verify authenticated scope/generation,
   startup-empty provenance and the exact active process before returning1. */
typedef uint32_t (*Kh02FreshPermit)(void*);
typedef uintptr_t (*Kh02FreshCopy)(uintptr_t,uintptr_t);
/* Runs only for the one authenticated New Game copy, after the game's copy
   completed and before the receipt owner can be admitted. Arguments are the
   actual primary/checkpoint structured-state pointers, NOT serialized slots. */
typedef uint32_t (*Kh02FreshInitialize)(void*,uintptr_t,uintptr_t);
typedef struct Kh02FreshConfig {
    uint64_t base;
    Kh02BoundedRead read;void *read_context;
    Kh02FreshCopy original;
    Kh02FreshPermit permit;void *permit_context;
    Kh02FreshInitialize initialize;void *initialize_context;
} Kh02FreshConfig;
KH02_RECEIPT_API Kh02FreshEntry *Kh02FreshCreate(const Kh02FreshConfig*,const Kh02ReceiptScope*,
    const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t,const unsigned char*,uint32_t);
KH02_RECEIPT_API uintptr_t Kh02FreshCopyInvoke(Kh02FreshEntry*,uintptr_t caller,uintptr_t destination,uintptr_t source);
KH02_RECEIPT_API Kh02ReceiptOwner *Kh02FreshCurrent(Kh02FreshEntry*);
/* Must run BEFORE every Load/Restore, scope loss or shutdown in future owner
   composition. Revocation is permanent; no reset/adopt-unknown API exists. */
KH02_RECEIPT_API void Kh02FreshRevoke(Kh02FreshEntry*);
KH02_RECEIPT_API uint32_t Kh02FreshDestroy(Kh02FreshEntry**);
#endif
