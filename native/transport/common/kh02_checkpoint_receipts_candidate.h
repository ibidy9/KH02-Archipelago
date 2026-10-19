#ifndef KH02_CHECKPOINT_RECEIPTS_CANDIDATE_H
#define KH02_CHECKPOINT_RECEIPTS_CANDIDATE_H
#include "kh02_receipt_owner_candidate.h"
#include "kh02_restore_snapshot_candidate.h"
typedef struct Kh02CheckpointReceipts Kh02CheckpointReceipts;
typedef Kh02ReceiptOwner *(*Kh02CheckpointCurrent)(void*);
typedef struct Kh02CheckpointConfig {
    uint64_t base;Kh02BoundedRead read;void *read_context;
    uintptr_t (*capture)(uintptr_t);
    uintptr_t (*restore)(uintptr_t,uint32_t);
    Kh02CheckpointCurrent current;void *current_context;
    void (*invalidate_current)(void*); /* Thread-safe lifecycle revocation only. */
} Kh02CheckpointConfig;
typedef struct Kh02CheckpointEvidence {
    uint64_t capture_id,context;
    uint32_t status,receipt_size;
    unsigned char receipts[KH02_RECEIPT_MAX_BYTES];
} Kh02CheckpointEvidence;
/* Standalone offline wrapper. All callback/owner lifetimes outlive it. */
KH02_RECEIPT_API Kh02CheckpointReceipts *Kh02CheckpointCreate(const Kh02CheckpointConfig*);
KH02_RECEIPT_API uintptr_t Kh02CheckpointCapture(Kh02CheckpointReceipts*,uintptr_t);
KH02_RECEIPT_API uintptr_t Kh02CheckpointRestore(Kh02CheckpointReceipts*,uintptr_t,uint32_t);
/* Evidence is single-consumption; not a freshly adopted branch or grant permit. */
KH02_RECEIPT_API uint32_t Kh02CheckpointTake(Kh02CheckpointReceipts*,Kh02CheckpointEvidence*);
/* Lifecycle must close this component BEFORE every disk load or branch change.
   Create a fresh component for the new branch; never reset this one. */
KH02_RECEIPT_API void Kh02CheckpointInvalidate(Kh02CheckpointReceipts*);
KH02_RECEIPT_API uint32_t Kh02CheckpointDestroy(Kh02CheckpointReceipts**);
#endif
