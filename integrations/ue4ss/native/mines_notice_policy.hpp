#pragma once
#include <cstdint>

// One banner per approach, with hysteresis and a cooldown. No queued callbacks.
struct MinesNoticePolicy {
    bool in_range{};
    uint64_t last{};

    void leave() {
        in_range = false;
    }

    bool ready(float distance, uint64_t now) {
        if (!(distance >= 0 && distance < 100000))
            return false;
        if (distance > 700) {
            leave();
            return false;
        }
        return distance <= 400 && !in_range && (!last || now - last >= 15000);
    }

    void shown(uint64_t now) {
        in_range = true;
        last = now;
    }
};
