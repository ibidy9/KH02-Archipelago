#ifndef KH02_DISK_LOAD_CANDIDATE_H
#define KH02_DISK_LOAD_CANDIDATE_H
#include "kh02_disk_lookup_candidate.h"
#include "kh02_save_snapshot_candidate.h"
typedef struct Kh02DiskLoad Kh02DiskLoad;
typedef unsigned char (*Kh02DiskLoadOriginal)(uintptr_t,uint32_t);
typedef struct Kh02DiskLoadConfig {
    uint64_t base,cookie,index_load_id;
    Kh02BoundedRead read;void *read_context;
    Kh02DiskLoadOriginal original;Kh02DiskLookup *lookup;
} Kh02DiskLoadConfig;
typedef struct Kh02DiskLoadResult {
    uint64_t generation;
    uint32_t status,index,receipt_size,reserved;
    unsigned char receipts[KH02_RECEIPT_MAX_BYTES];
} Kh02DiskLoadResult;
enum {KH02_LOAD_KNOWN=1,KH02_LOAD_UNKNOWN,KH02_LOAD_BAD_SNAPSHOT,KH02_LOAD_FAILED,
      KH02_LOAD_CLOSED,KH02_LOAD_WRONG_THREAD,KH02_LOAD_REENTRANT,KH02_LOAD_EXHAUSTED};
/* Offline adapter around the established load signature; no entry patching.
   Lookup/read/original lifetimes outlive this component and all invocations.
   Result is evidence, not an active AP owner or permission to replay. */
KH02_RECEIPT_API Kh02DiskLoad *Kh02DiskLoadCreate(const Kh02DiskLoadConfig*);
KH02_RECEIPT_API unsigned char Kh02DiskLoadInvoke(Kh02DiskLoad*,uintptr_t,uint32_t,Kh02DiskLoadResult*);
/* Same owner thread, outside the callback: checks current evidence freshness,
   not readiness or authorization to apply items. */
KH02_RECEIPT_API uint32_t Kh02DiskLoadCheck(Kh02DiskLoad*,uint64_t);
KH02_RECEIPT_API void Kh02DiskLoadStop(Kh02DiskLoad*);
/* External callers must be excluded/joined before final destruction. */
KH02_RECEIPT_API uint32_t Kh02DiskLoadDestroy(Kh02DiskLoad**);
#endif
