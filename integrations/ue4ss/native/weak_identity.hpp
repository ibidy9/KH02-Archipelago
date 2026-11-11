#pragma once
#include "object_identity.hpp"

namespace kh02_host {
// Ref::Get must implement the SDK slot/serial/validity contract. Never cast
// an expected integer into an object or dereference it to check validity.
template <typename Ref, typename Validator>
auto checked_weak_identity(const Ref &ref, std::uint64_t expected, Validator validate) {
    return validate_identity(ref.Get(), expected, validate);
}
} // namespace kh02_host
