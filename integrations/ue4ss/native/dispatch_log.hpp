#pragma once
#include <cstdint>
#include "sdk_pump_api.h"

namespace kh02_sdk {
// Diagnostics only. The caller always pumps/validates first, regardless of this
// decision. Fixed state; no request queue, timer thread or authority changes.
struct DispatchLog {
    bool seen{};
    std::uint64_t last_emit{};
    std::uint32_t status{}, bootstrap{}, observed{}, emitted{};

    bool observe(const Kh02SdkPumpStatus &sample, std::uint64_t now) noexcept {
        const bool important = !seen || sample.core_status != status ||
                               sample.window_commands != bootstrap ||
                               sample.sdk_commands < observed;
        const bool periodic =
            seen && sample.sdk_commands != emitted && now >= last_emit && now - last_emit >= 5000;
        status = sample.core_status;
        bootstrap = sample.window_commands;
        observed = sample.sdk_commands;
        if (!important && !periodic)
            return false;
        seen = true;
        last_emit = now;
        emitted = sample.sdk_commands;
        return true;
    }
};
} // namespace kh02_sdk
