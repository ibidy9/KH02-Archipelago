#ifndef KH02_RECEIPT_BANK_CANDIDATE_H
#define KH02_RECEIPT_BANK_CANDIDATE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#define KH02_RECEIPT_MAX_ITEMS 4096u
#define KH02_RECEIPT_MAX_BYTES 102456u
#define KH02_RECEIPT_API __declspec(dllexport)
typedef struct Kh02ReceiptIdentity {uint64_t index,item,player;} Kh02ReceiptIdentity;
typedef struct Kh02ReceiptScope {
    uint64_t cookie,load_id;
    uint32_t team,slot;
    const unsigned char *seed;
    uint32_t seed_size,reserved;
} Kh02ReceiptScope;
typedef struct Kh02ReceiptLease {uint64_t instance,serial,revision;} Kh02ReceiptLease;
typedef struct Kh02ReceiptBank Kh02ReceiptBank;
enum { KH02_BANK_OK=1,KH02_BANK_REFUSED,KH02_BANK_BUSY,KH02_BANK_CAPACITY,KH02_BANK_CLOSED };
/* Offline ownership component. No native hooks, game calls, durability or load
   proof. All buffers are already-owned local inputs. Caller joins all API users
   before destruction; workers may read/release held leases after close begins. */
KH02_RECEIPT_API Kh02ReceiptBank *Kh02ReceiptBankCreate(const Kh02ReceiptScope*,const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t);
KH02_RECEIPT_API uint32_t Kh02ReceiptBankHistory(Kh02ReceiptBank*,const Kh02ReceiptIdentity*,uint32_t);
/* First baseline is explicit trusted evidence supplied by a future load adapter.
   A proposal is parent plus exactly one supported receipt. Neither operation
   selects an active revision or claims an effect has occurred. */
KH02_RECEIPT_API uint32_t Kh02ReceiptBankBaseline(Kh02ReceiptBank*,const unsigned char*,uint32_t);
KH02_RECEIPT_API uint32_t Kh02ReceiptBankPropose(Kh02ReceiptBank*,uint64_t,const unsigned char*,uint32_t);
/* Extract the one added identity from two already validated, owned sets. */
KH02_RECEIPT_API uint32_t Kh02ReceiptBankAdded(Kh02ReceiptBank*,uint64_t,uint64_t,Kh02ReceiptIdentity*);
KH02_RECEIPT_API uint32_t Kh02ReceiptBankAcquire(Kh02ReceiptBank*,uint64_t,Kh02ReceiptLease*);
KH02_RECEIPT_API uint32_t Kh02ReceiptBankRead(Kh02ReceiptBank*,const Kh02ReceiptLease*,unsigned char*,uint32_t,uint32_t*);
KH02_RECEIPT_API uint32_t Kh02ReceiptBankRelease(Kh02ReceiptBank*,Kh02ReceiptLease*);
KH02_RECEIPT_API uint32_t Kh02ReceiptBankDestroy(Kh02ReceiptBank**);
#endif
