#ifndef KH02_SAVE_SNAPSHOT_CANDIDATE_H
#define KH02_SAVE_SNAPSHOT_CANDIDATE_H
#include "kh02_owner_reader.h"
#define KH02_SAVE_RECORD_SIZE 0x3B40u
typedef struct Kh02SaveSnapshot {
    uint64_t address;
    uint32_t index,status;
    unsigned char manager[48];
    unsigned char record[KH02_SAVE_RECORD_SIZE];
#ifdef KH02_SAVE_HEADER_BINDING
    unsigned char header[0xb0];
#endif
} Kh02SaveSnapshot;
enum { KH02_SNAPSHOT_OK=1,KH02_SNAPSHOT_BAD_SELECTION,KH02_SNAPSHOT_BAD_LAYOUT,
       KH02_SNAPSHOT_READ_FAILED,KH02_SNAPSHOT_CHANGED };
/* Exact reads through the established callback type. No memory discovery,
   native calls, save writes or proof of receipt membership/readiness. */
uint32_t Kh02SaveSnapshotRead(Kh02BoundedRead,void*,uint64_t,uint64_t,uint32_t,Kh02SaveSnapshot*);
#endif
