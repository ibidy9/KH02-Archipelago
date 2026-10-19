#ifndef KH02_STAT_SUPPRESSION_V109_H
#define KH02_STAT_SUPPRESSION_V109_H
#include <stdint.h>
typedef int (*Kh02BonusReadV109)(void*,uint64_t,void*,uint32_t);
typedef struct Kh02BonusRequestV109 {
    uint64_t base,context,subrecord,name;
    uint32_t kind,authorized;
    int32_t objective;
} Kh02BonusRequestV109;
/* 1 suppress; 0 forward unchanged. This function has no write callback. */
uint32_t Kh02BonusSuppressV109(Kh02BonusReadV109,void*,const Kh02BonusRequestV109*);
#endif
