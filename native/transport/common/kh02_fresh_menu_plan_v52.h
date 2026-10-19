#ifndef KH02_FRESH_MENU_PLAN_V52_H
#define KH02_FRESH_MENU_PLAN_V52_H
#include <stdint.h>
#include <string.h>

/* Offline candidate only. Caller must establish fresh ownership, validate both
 * complete records, stage both plans, and provide verified commit/rollback.
 * Never call this on a loaded save. No native UI call or process access here.
 * Objective availability remains the independent proven v50 initializer.
 */
static int Kh02FreshMenuPlanV52(const unsigned char *before,
                              unsigned char *after, uint32_t size) {
    static const uint32_t commands[4] = {0x33, 0x37, 0x3b, 0x47};
    uint32_t i, command;
    if (!before || !after || before == after || size != 0x3d28u) return 0;
    /* Fresh defaults or our exact idempotent values only. Reject unknown
       command flags and user-selected shortcuts rather than replacing them. */
    if ((before[0x3c58] & ~0x26u) || (before[0x3c59] & ~3u)) return 0;
    for (i=0; i<4; ++i) {
        memcpy(&command, before+0x3c68+i*4, sizeof(command));
        if (command && command != commands[i]) return 0;
    }
    memcpy(after, before, size);
    /* v53: do not manufacture tutorial/story history before Main Road.
       Native tutorial progression owns 0x3200/0x3201. */
    after[0x3c58] |= 0x26;
    after[0x3c59] |= 3;
    for (i=0; i<4; ++i)
        memcpy(after+0x3c68+i*4, &commands[i], sizeof(commands[i]));
    return 1;
}
#endif
