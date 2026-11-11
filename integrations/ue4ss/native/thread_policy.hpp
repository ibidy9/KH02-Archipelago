#pragma once
#include <cstdint>

namespace kh02_host {
struct ThreadEvidence {
    std::uint32_t process, callback_thread, window_process, window_thread, windows;
};

inline bool same_owner_thread(const ThreadEvidence &e) noexcept {
    return e.process && e.callback_thread && e.windows == 1 && e.window_process == e.process &&
           e.window_thread == e.callback_thread;
}
} // namespace kh02_host
