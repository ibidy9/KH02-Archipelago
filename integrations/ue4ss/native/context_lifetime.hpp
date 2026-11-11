#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace kh02_host {
constexpr std::size_t max_context_name = 4096;

inline bool advance_context(std::uint64_t &revision) {
    if (revision == std::numeric_limits<std::uint64_t>::max())
        return false;
    ++revision;
    return true;
}

class ObservationLease {
    std::atomic_flag &flag;
    bool held;

  public:
    explicit ObservationLease(std::atomic_flag &f)
        : flag(f), held(!f.test_and_set(std::memory_order_acquire)) {}

    ~ObservationLease() {
        if (held)
            flag.clear(std::memory_order_release);
    }

    explicit operator bool() const {
        return held;
    }

    ObservationLease(const ObservationLease &) = delete;
    ObservationLease &operator=(const ObservationLease &) = delete;
};
} // namespace kh02_host
