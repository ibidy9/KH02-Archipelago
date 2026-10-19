#ifndef KH02_RECEIPT_GRANT_CANDIDATE_H
#define KH02_RECEIPT_GRANT_CANDIDATE_H
#include "kh02_receipt_session_candidate.h"
#include "kh02_inventory_candidate.h"
typedef struct Kh02ReceiptItemName {uint32_t index,number;} Kh02ReceiptItemName;
typedef uint32_t (*Kh02GrantCheck)(void*);
typedef uint32_t (*Kh02GrantRead)(void*,Kh02SessionSample*,Kh02InventorySnapshot*);
typedef uint32_t (*Kh02GrantConvert)(void*,uint32_t,Kh02ReceiptItemName*);
typedef void (*Kh02GrantNative)(void*,const Kh02ReceiptItemName*,uint32_t);
/* Offline adapter boundary. A future native owner supplies exact bounded reads,
   verified converter/grant addresses, build guards and active/fault checks.
   None are discovered or hooked here. Check must include those build guards. */
typedef struct Kh02ReceiptGrant {
    Kh02ReceiptOwner *owner;
    Kh02SessionCandidate *session;
    Kh02Lease lease;
    Kh02InventorySnapshot before;
    Kh02GrantCheck check;
    Kh02GrantRead read;
    Kh02GrantConvert convert;
    Kh02GrantNative grant;
    void *context;
    uint32_t show,entered,observed_before,observed_after;
} Kh02ReceiptGrant;
KH02_RECEIPT_API uint32_t Kh02ReceiptGrantExecute(uint32_t,const Kh02ReceiptIdentity*,void*);
KH02_RECEIPT_API uint32_t Kh02LocalConsumableGrant193(uint32_t,Kh02ReceiptGrant*);
#endif
