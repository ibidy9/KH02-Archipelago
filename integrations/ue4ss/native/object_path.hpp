#pragma once
#include <string>
#include <string_view>

namespace kh02_host {
inline std::string copied_object_path(std::string_view full) {
    if (full.empty() || full.size() > 512 || full.find_first_of("\r\n") != std::string_view::npos)
        return {};
    const auto separator = full.find(' ');
    if (separator == std::string_view::npos)
        return {};
    const auto path = full.substr(separator + 1);
    if (!path.starts_with("/Game/Levels/") ||
        path.find(":PersistentLevel.") == std::string_view::npos)
        return {};
    return std::string(path);
}
} // namespace kh02_host
