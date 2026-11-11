#include <stdint.h>
#include <stdio.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
#include "../../../native/transport/common/menu_return_v215.h"
static unsigned char memory[0x8000], original[0x8000];
static unsigned writes, fail_at, persistent, allowed = 1, permit_calls, change_on_recheck,
                                             deny_on_recheck;

static int rd(void *p, uint64_t address, void *out, uint32_t size) {
    (void)p;
    if (address + size > sizeof(memory))
        return 0;
    memcpy(out, memory + address, size);
    return 1;
}

static int wr(void *p, uint64_t address, const void *in, uint32_t size) {
    (void)p;
    writes++;
    assert(address + size <= sizeof(memory));
    if (fail_at && (writes == fail_at || (persistent && writes >= fail_at))) {
        memcpy(memory + address, in, size / 2);
        return 0;
    }
    memcpy(memory + address, in, size);
    return 1;
}

static int permit(void *p) {
    (void)p;
    permit_calls++;
    if (permit_calls == 2 && change_on_recheck)
        memory[0x3538] ^= 1;
    return allowed && !(permit_calls == 2 && deny_on_recheck);
}

static void put(uint32_t offset, uint32_t value) {
    memcpy(memory + offset, &value, 4);
}

static uint32_t get(uint32_t offset) {
    uint32_t v;
    memcpy(&v, memory + offset, 4);
    return v;
}

static void setup(void) {
    memset(memory, 0xa5, sizeof(memory));
    for (unsigned i = 0; i < 2; i++) {
        unsigned d = i * 0x3d28;
        put(d + 0x3338, i ? 340 : 350);
        put(d + 0x3848, 0xabcddcba);
        memset(memory + d + 0x3538, 0xff, 6);
        memset(memory + d + 0x384c, 0, 32);
        strcpy((char *)memory + d + 0x384c, "/Game/Levels/dw/dw_08/dw_08");
    }
    memcpy(original, memory, sizeof(memory));
    writes = fail_at = persistent = permit_calls = change_on_recheck = deny_on_recheck = 0;
    allowed = 1;
}

int main(void) {
    for (unsigned mode = 0; mode < 3; mode++) {
        setup();
        uint32_t mask = mode ? 0 : 0x18;
        assert(menu_prepare214(rd, wr, permit, 0, 0, mode, mask) == 1);
        for (unsigned i = 0; i < 2; i++) {
            unsigned d = i * 0x3d28;
            assert(get(d + 0x3338) == (mode == 0 ? 280 : mode == 1 ? (i ? 340 : 350) : 250));
            assert(get(d + 0x3848) == (0xabcddcba & ~mask));
            if (mode == 1)
                assert(!memcmp(memory + d + 0x3538, original + d + 0x3538, 6));
            if (mode == 2) {
                assert(memory[d + 0x353a] == 0xf1);
                assert(memory[d + 0x353d] == 0xf7);
            }
            if (mode == 0) {
                assert(memory[d + 0x353b] == 0x6f);
                assert(memory[d + 0x353c] == 0xe1);
                assert(memory[d + 0x353d] == 0xcf);
            }
            /* Remove only authorized spans, then compare every remaining byte. */
            memcpy(memory + d + 0x3338, original + d + 0x3338, 4);
            if (mode != 1)
                memcpy(memory + d + 0x3538, original + d + 0x3538, 6);
            if (mode == 0)
                memcpy(memory + d + 0x3848, original + d + 0x3848, 4);
        }
        assert(!memcmp(memory, original, sizeof(memory)));
        unsigned steps = mode == 0 ? 6 : mode == 1 ? 0 : 4;
        for (unsigned failure = 1; failure <= steps; failure++) {
            setup();
            fail_at = failure;
            assert(menu_prepare214(rd, wr, permit, 0, 0, mode, mask) == 2);
            assert(!memcmp(memory, original, sizeof(memory)));
        }
    }
    setup();
    allowed = 0;
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 0);
    assert(!writes);
    setup();
    put(0x3338, 323);
    put(0x7060, 322);
    assert(menu_prepare214(rd, wr, permit, 0, 0, 0, 0x18) == 1);
    setup();
    put(0x3338, 301);
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 0);
    assert(!writes);
    setup();
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 8) == 0);
    assert(!writes);

    for (unsigned story = 323; story <= 360; story++) {
        setup();
        put(0x3338, story);
        put(0x7060, story);
        memcpy(original, memory, sizeof(memory));
        assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 1);
        assert(!writes);
        assert(!memcmp(memory, original, sizeof(memory)));
    }
    setup();
    put(0x3338, 323);
    put(0x7060, 322);
    memcpy(original, memory, sizeof(memory));
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 1);
    assert(!writes);
    assert(!memcmp(memory, original, sizeof(memory)));
    /* Cross-world routing retains the established Forest destination profile. */
    setup();
    for (unsigned i = 0; i < 2; i++) {
        unsigned d = i * 0x3d28;
        put(d + 0x3338, 280);
        strcpy((char *)memory + d + 0x384c, "/Game/Levels/dw/dw_03/dw_03");
    }
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 1);
    assert(writes == 2);
    assert(get(0x3338) == 321 && get(0x7060) == 321);
    setup();
    for (unsigned i = 0; i < 2; i++) {
        unsigned d = i * 0x3d28;
        put(d + 0x3338, 280);
        strcpy((char *)memory + d + 0x384c, "/Game/Levels/dw/dw_03/dw_03");
    }
    fail_at = 2;
    persistent = 1;
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 3);

    setup();
    change_on_recheck = 1;
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 0);
    assert(!writes);
    setup();
    deny_on_recheck = 1;
    assert(menu_prepare214(rd, wr, permit, 0, 0, 1, 0) == 0);
    assert(!writes);
    for (unsigned mode = 3; mode <= 4; mode++) {
        setup();
        assert(menu_prepare214(rd, wr, permit, 0, 0, mode, 0) == 1);
        assert(get(0x3338) == (mode == 3 ? 150 : 340));
        assert(get(0x7060) == (mode == 3 ? 150 : 340));
    }
    /* Foreign-world rail entry must reuse earned Forest milestones, not321. */
    const uint32_t bits[3] = {1, 2, 4};
    const char *maps[] = {"/Game/Levels/dw/dw_02/dw_02", "/Game/Levels/dw/dw_03/dw_03",
                          "/Game/Levels/dw/dw_09/dw_09"};
    for (unsigned world = 0; world < 3; world++) {
        for (unsigned complete = 0; complete < 2; complete++) {
            setup();
            for (unsigned i = 0; i < 2; i++) {
                unsigned d = i * 0x3d28;
                put(d + 0x3338, world == 0 ? 150 : world == 1 ? 280 : 410);
                strcpy((char *)memory + d + 0x384c, maps[world]);
                put(d + 0x3848, complete ? 7 : 3);
                memory[d + 0x353d] = complete ? 0x20 : 0;
            }
            memcpy(original, memory, sizeof(memory));
            assert(menu_rail_return215(rd, wr, permit, 0, 0, bits) == 1);
            assert(get(0x3338) == (complete ? 350 : 323));
            assert(get(0x7060) == (complete ? 350 : 323));
            for (unsigned i = 0; i < 2; i++)
                memcpy(memory + i * 0x3d28 + 0x3338, original + i * 0x3d28 + 0x3338, 4);
            assert(!memcmp(memory, original, sizeof(memory)));
        }
    }
    setup();
    put(0x3338, 360);
    put(0x7060, 360);
    assert(menu_rail_return215(rd, wr, permit, 0, 0, bits) == 1);
    assert(!writes);
    for (unsigned failure = 1; failure <= 2; failure++) {
        setup();
        for (unsigned i = 0; i < 2; i++) {
            unsigned d = i * 0x3d28;
            put(d + 0x3338, 280);
            strcpy((char *)memory + d + 0x384c, maps[1]);
            put(d + 0x3848, 7);
        }
        memcpy(original, memory, sizeof(memory));
        fail_at = failure;
        assert(menu_rail_return215(rd, wr, permit, 0, 0, bits) == 2);
        assert(!memcmp(memory, original, sizeof(memory)));
    }
    setup();
    for (unsigned i = 0; i < 2; i++) {
        unsigned d = i * 0x3d28;
        put(d + 0x3338, 280);
        strcpy((char *)memory + d + 0x384c, maps[1]);
        put(d + 0x3848, 1);
    }
    assert(menu_rail_return215(rd, wr, permit, 0, 0, bits) == 0);
    assert(!writes);
    /* All discovery subsets, all destinations: later implies earlier only. */
    for (unsigned mask = 0; mask < 8; mask++) {
        uint32_t flags = 0;
        for (unsigned i = 0; i < 3; i++)
            if (mask & (1u << i))
                flags |= bits[i];
        uint32_t expected = (mask & 4) ? 7 : (mask & 2) ? 3 : (mask & 1);
        assert(forest_reachable_saves(flags | 0x80000000u, bits) == (expected | 0x80000000u));
        for (unsigned selected = 0; selected < 3; selected++) {
            setup();
            for (unsigned i = 0; i < 2; i++) {
                unsigned d = i * 0x3d28;
                put(d + 0x3338, 280);
                strcpy((char *)memory + d + 0x384c, maps[1]);
                put(d + 0x3848, flags | 0x80000000u);
                memory[d + 0x353d] = 0;
            }
            memcpy(original, memory, sizeof(memory));
            int result = menu_return215(rd, wr, permit, 0, 0, selected, bits);
            if (expected & bits[selected]) {
                assert(result == 1);
                assert(get(0x3338) == ((mask & 4) ? 350 : (mask & 2) ? 323 : 300));
            } else {
                assert(result == 0);
                assert(!writes);
            }
            for (unsigned i = 0; i < 2; i++)
                memcpy(memory + i * 0x3d28 + 0x3338, original + i * 0x3d28 + 0x3338, 4);
            assert(!memcmp(memory, original, sizeof(memory)));
        }
    }
    /* Clearing later discovery on a real restart does not retain inferred bits. */
    assert(forest_reachable_saves(7 & ~(bits[1] | bits[2]), bits) == bits[0]);
    assert(forest_reachable_saves(6 & ~(bits[1] | bits[2]), bits) == 0);
    puts(
        "PASS Forest discovery-only reset delta; rail story preserved with zero writes; legacy Phantom profile; byte preservation and partial-write rollback at every step");
    return 0;
}
