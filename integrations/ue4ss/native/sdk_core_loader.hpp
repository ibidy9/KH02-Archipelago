#pragma once
#include <Windows.h>
#include <bcrypt.h>
#include <string>
#include <cstring>
#include <cstddef>
#include "sdk_pump_api.h"

namespace kh02_sdk {
template <std::size_t N>
bool approved_core_file(const std::wstring &path, const char *const (&accepted)[N]) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    bool ok = false;
    do {
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 64 * 1024 * 1024)
            break;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
            break;
        if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0)
            break;
        unsigned char bytes[4096], digest[32];
        DWORD count{};
        std::uint64_t total = 0;
        bool valid = true;
        for (;;) {
            if (!ReadFile(file, bytes, sizeof(bytes), &count, nullptr)) {
                valid = false;
                break;
            }
            if (!count)
                break;
            total += count;
            if (total > 64 * 1024 * 1024 || BCryptHashData(hash, bytes, count, 0) < 0) {
                valid = false;
                break;
            }
        }
        if (!valid || total != static_cast<std::uint64_t>(size.QuadPart) ||
            BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0)
            break;
        constexpr char hex[] = "0123456789abcdef";
        char text[65]{};
        for (unsigned i = 0; i < 32; ++i) {
            text[2 * i] = hex[digest[i] >> 4];
            text[2 * i + 1] = hex[digest[i] & 15];
        }
        for (const auto *expected : accepted)
            if (std::strcmp(text, expected) == 0) {
                ok = true;
                break;
            }
    } while (false);
    if (hash)
        BCryptDestroyHash(hash);
    if (algorithm)
        BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return ok;
}

enum class CoreState { not_loaded, refused, ready };

struct Core {
    CoreState state{CoreState::not_loaded};
    Kh02SdkPumpFn pump{};
};

inline std::wstring runtime_core_path() {
    wchar_t directory[32768], canonical[32768];
    const DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", directory, 32768);
    if (!count || count >= 32768)
        return {};
    const auto joined =
        std::wstring(directory, count) + L"\\KH02AP\\runtime\\Kh02FreshTransport.dll";
    const DWORD n = GetFullPathNameW(joined.c_str(), 32768, canonical, nullptr);
    return n && n < 32768 ? std::wstring(canonical, n) : std::wstring{};
}

template <std::size_t N>
Core loaded_core(const std::wstring &path, const char *const (&accepted)[N]) {
    if (path.empty())
        return {CoreState::refused, {}};
    HMODULE module = GetModuleHandleW(path.c_str());
    if (!module)
        return {};
    wchar_t actual[32768];
    const DWORD n = GetModuleFileNameW(module, actual, 32768);
    if (!n || n >= 32768 || CompareStringOrdinal(actual, n, path.c_str(), -1, TRUE) != CSTR_EQUAL ||
        !approved_core_file(path, accepted))
        return {CoreState::refused, {}};
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, path.c_str(), &pinned) ||
        pinned != module)
        return {CoreState::refused, {}};
    auto address = GetProcAddress(module, "Kh02SdkOwnerPump");
    if (!address)
        return {CoreState::refused, {}};
    Kh02SdkPumpFn pump{};
    static_assert(sizeof(address) == sizeof(pump));
    std::memcpy(&pump, &address, sizeof(address));
    return {CoreState::ready, pump};
}
} // namespace kh02_sdk
