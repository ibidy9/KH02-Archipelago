#pragma once
#include <cstdint>

namespace kh02_host {
// A named lookup may return an old cached address. Compare with this event's
// independently obtained identity before calling anything that reads the object.
template <typename Object, typename Validate>
Object *validate_identity(Object *found, std::uint64_t expected, Validate validate) {
    if (!found || !expected || reinterpret_cast<std::uintptr_t>(found) != expected)
        return nullptr;
    return validate(found) ? found : nullptr;
}
} // namespace kh02_host
