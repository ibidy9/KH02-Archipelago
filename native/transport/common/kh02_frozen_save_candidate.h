#ifndef KH02_FROZEN_SAVE_CANDIDATE_H
#define KH02_FROZEN_SAVE_CANDIDATE_H
#include "kh02_owner_reader.h"
#define KH02_FULL_SAVE_SIZE 0x1725B0u
typedef struct Kh02FrozenSave {
    /* Native TArray-compatible descriptor. Storage belongs only to this object. */
    const unsigned char *data;
    uint32_t length,capacity;
} Kh02FrozenSave;
enum { KH02_FREEZE_OK=1,KH02_FREEZE_BAD_INPUT,KH02_FREEZE_READ_FAILED,
       KH02_FREEZE_CHANGED,KH02_FREEZE_NO_MEMORY,KH02_FREEZE_PROTECT_FAILED };
/* Must supply a zero-initialized owner. Never reuses an occupied owner.
   Caller must keep it alive through the entire synchronous original save call.
   Stable repeated reads are not proof of per-record AP membership or atomicity. */
__declspec(dllexport) uint32_t Kh02FreezeSave(Kh02BoundedRead,void*,uint64_t,uint64_t,Kh02FrozenSave*);
__declspec(dllexport) uint32_t Kh02ReleaseFrozenSave(Kh02FrozenSave*);
#endif
