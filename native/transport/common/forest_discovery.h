#ifndef KH02_FOREST_DISCOVERY_H
#define KH02_FOREST_DISCOVERY_H
#include <stdint.h>

/* Reaching a later Forest save makes earlier saves reachable. This is a
 * read-only view: do not manufacture persistent discovery or later progress. */
static inline uint32_t forest_reachable_saves(uint32_t discovered, const uint32_t bits[3]) {
    if (discovered & bits[2])
        discovered |= bits[1] | bits[0];
    else if (discovered & bits[1])
        discovered |= bits[0];
    return discovered;
}
#endif
