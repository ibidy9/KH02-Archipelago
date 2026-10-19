#ifndef KH02_GOAL_CONFIG_V80_H
#define KH02_GOAL_CONFIG_V80_H
#include <stdint.h>
typedef struct Kh02GoalConfigV80 {
    uint64_t sequence,epoch;
    uint32_t configured,goal,requirement,start,schema,access,available,reserved;
} Kh02GoalConfigV80;
_Static_assert(sizeof(Kh02GoalConfigV80)==48,"goal config ABI");
#endif
