#ifndef KH02_DIFFICULTY_PENDING_CANDIDATE_H
#define KH02_DIFFICULTY_PENDING_CANDIDATE_H
#include <stdint.h>
#define DIFF_API __declspec(dllexport)
typedef struct Kh02DifficultyRequest {
    uint32_t magic,version,size,desired;
    uint64_t sequence,cookie,challenge,authority,epoch;
    unsigned char scope[32];
} Kh02DifficultyRequest;
typedef struct Kh02DifficultyPending {
    uint64_t cookie,challenge;
    unsigned char scope[32];
    uint64_t sequence,authority,requested_epoch,applied_epoch;
    uint32_t desired,phase,result,reserved;
} Kh02DifficultyPending;
typedef uint32_t (*Kh02DifficultyWrite)(uintptr_t,uint32_t,uint32_t);
/* permission means supported build + exact context + current known owner +
   owner thread + current AP authority, supplied by the existing coordinator.
   These functions do not establish permission or discover game objects. */
DIFF_API uint32_t Kh02DifficultyBind(Kh02DifficultyPending*,uint64_t,uint64_t,const unsigned char*);
DIFF_API uint32_t Kh02DifficultyQueue(Kh02DifficultyPending*,const Kh02DifficultyRequest*,uint64_t,uint64_t,uint32_t);
DIFF_API uint32_t Kh02DifficultyApply(Kh02DifficultyPending*,uint64_t,uint64_t,uint32_t,uintptr_t,uint32_t,Kh02DifficultyWrite);
DIFF_API uint32_t Kh02DifficultyConfirm(Kh02DifficultyPending*,uint64_t,uint64_t,uint32_t,uint64_t);
#endif
