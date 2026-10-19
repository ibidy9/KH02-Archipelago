#ifndef KH02_OWNER_READER_H
#define KH02_OWNER_READER_H
#include "kh02_session_candidate.h"
typedef int (*Kh02BoundedRead)(void*,uint64_t,void*,uint32_t);
/* Fixed reads only. Callback must return success only for an exact-length read. */
KH02_GATE_API uint32_t Kh02OwnerReadSample(Kh02BoundedRead,void*,uint64_t,uint64_t,
    uint64_t,Kh02SessionSample*);
#endif
