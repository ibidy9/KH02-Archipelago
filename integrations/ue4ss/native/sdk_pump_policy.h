#pragma once
#include <stdint.h>

typedef struct Kh02SdkDispatch {
    uint32_t bound, sdk_commands, window_commands, ignored_wakes;
} Kh02SdkDispatch;

static inline int Kh02SdkWindowAllowed(const Kh02SdkDispatch *p, uint32_t command) {
    /* Never silently execute a gameplay receipt through the bootstrap path. */
    return p && !p->bound && command != 5;
}

static inline int Kh02SdkCanBind(uint32_t current, uint32_t owner, uint32_t setup_ok, uint32_t busy,
                                 uint32_t active) {
    return current && current == owner && setup_ok == 1 && !busy && active;
}
