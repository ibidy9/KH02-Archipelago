#ifndef KH02_RESTORE_SNAPSHOT_CANDIDATE_H
#define KH02_RESTORE_SNAPSHOT_CANDIDATE_H
#include "kh02_owner_reader.h"
#define KH02_RESTORE_INVENTORY_SIZE 0x1880u
typedef struct Kh02RestoreSnapshot {
    uint64_t context;
    uint32_t status,reserved;
    unsigned char primary[KH02_RESTORE_INVENTORY_SIZE];
    unsigned char checkpoint[KH02_RESTORE_INVENTORY_SIZE];
} Kh02RestoreSnapshot;
enum { KH02_RESTORE_OK=1,KH02_RESTORE_BAD_CONTEXT,KH02_RESTORE_READ_FAILED,
       KH02_RESTORE_CHANGED };
/* Context must come from the classified native parent entry, never discovery.
   This is observation only, not receipt identity or permission to deliver. */
uint32_t Kh02RestoreSnapshotRead(Kh02BoundedRead,void*,uint64_t,uint64_t,Kh02RestoreSnapshot*);
#endif
