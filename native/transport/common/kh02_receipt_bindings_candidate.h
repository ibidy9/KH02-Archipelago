#ifndef KH02_RECEIPT_BINDINGS_CANDIDATE_H
#define KH02_RECEIPT_BINDINGS_CANDIDATE_H
#include "kh02_record_bindings_candidate.h"
#include "kh02_receipt_owner_candidate.h"
typedef struct Kh02ReceiptBindings Kh02ReceiptBindings;
typedef struct Kh02ReceiptEntry {uint64_t revision;uint32_t offset,size;} Kh02ReceiptEntry;
typedef struct Kh02ReceiptBundle {
    uint64_t instance,serial,cookie,load_id;
    uint32_t size,set_count;
    uint32_t team,slot,seed_size,seed_offset;
    uint64_t revisions[100];
    Kh02ReceiptEntry sets[64];
    unsigned char container[KH02_BIND_SIZE];
    /* Complete encoded sets follow. Offsets are from the start of this bundle.
       Zero record revision means unknown, never an empty AP receipt set. */
} Kh02ReceiptBundle;
typedef struct Kh02ReceiptBundleLease {const Kh02ReceiptBundle *data;uint64_t instance,serial;} Kh02ReceiptBundleLease;
/* Scope comes from an independently validated load association. Baseline record
   bytes start unknown. No load evidence or empty membership is inferred here. */
MEMBER_API Kh02ReceiptBindings *Kh02ReceiptBindingsCreate(const Kh02ReceiptScope*,const unsigned char*,uint32_t);
/* Caller supplies successful original-stage bytes and its completed stage lease.
   The caller retains that lease; this store copies and owns the receipt bytes. */
MEMBER_API uint32_t Kh02ReceiptBindingsPut(Kh02ReceiptBindings*,uint32_t,Kh02ReceiptOwner*,const Kh02ReceiptLease*,const unsigned char*,uint32_t);
MEMBER_API uint32_t Kh02ReceiptBindingsRefresh(Kh02ReceiptBindings*,uint32_t,const unsigned char*,const unsigned char*,uint32_t);
MEMBER_API uint32_t Kh02ReceiptBindingsCapture(Kh02ReceiptBindings*,const unsigned char*,uint32_t,Kh02ReceiptBundleLease*);
MEMBER_API uint32_t Kh02ReceiptBindingsRelease(Kh02ReceiptBindings*,Kh02ReceiptBundleLease*);
MEMBER_API uint32_t Kh02ReceiptBindingsInvalidate(Kh02ReceiptBindings*);
/* Owner thread only; join API users before destroying. Held bundles refuse
   destruction. Workers can release an old bundle after invalidation. */
MEMBER_API uint32_t Kh02ReceiptBindingsDestroy(Kh02ReceiptBindings**);
#endif
