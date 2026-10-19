#ifndef KH02_MAGIC_PREDICATE_V41_H
#define KH02_MAGIC_PREDICATE_V41_H
#include <stdint.h>
typedef struct Kh02MagicPredicate {
    uint64_t raw,reasons[10],accepted[7],blocked[7];
    uint64_t epoch,observed,mask,enabled,sequence;
    uint64_t last_receiver,last_actor,last_caller,last_command,last_grand;
} Kh02MagicPredicate;
uint32_t Kh02MagicGateSet(uint64_t,uint64_t,uint32_t,uint64_t);
void Kh02MagicPredicateSnapshot(Kh02MagicPredicate*);
#endif
