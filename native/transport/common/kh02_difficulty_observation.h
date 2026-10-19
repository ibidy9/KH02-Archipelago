#ifndef KH02_DIFFICULTY_OBSERVATION_H
#define KH02_DIFFICULTY_OBSERVATION_H
#include "kh02_owner_reader.h"
/* Read-only, bounded observation. Packed result: status low8, primary bits8..15,
   checkpoint bits16..23. No addresses recorded and no game functions called.
   Status1 means both values valid;2 input;3 read;4 identity;5 changed;6 enum. */
__declspec(dllexport) uint64_t Kh02DifficultyObserve(Kh02BoundedRead,void*,uint64_t,uint64_t);
__declspec(dllexport) uint64_t Kh02DifficultyObserveRoot(Kh02BoundedRead,void*,uint64_t);
#endif
