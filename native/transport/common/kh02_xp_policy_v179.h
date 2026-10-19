#ifndef KH02_XP_POLICY_V179_H
#define KH02_XP_POLICY_V179_H
#include <stdint.h>
/* Offline candidate only. No reads/writes/hooks. Scale the earned delta after
   native XP-zero/chance rules; preserve total cap and prevent signed overflow. */
static int32_t kh02_xp_delta179(int authorized, uint32_t halves,
                               int32_t current, int32_t earned) {
    uint64_t scaled, room;
    if (!authorized || earned <= 0 || current < 0 || current > 9999999 ||
        (halves != 2 && halves != 3 && halves != 4 && halves != 6 && halves != 8))
        return earned;
    room = 9999999u - (uint32_t)current;
    scaled = ((uint64_t)(uint32_t)earned * halves) / 2u;
    return (int32_t)(scaled < room ? scaled : room);
}
#endif
