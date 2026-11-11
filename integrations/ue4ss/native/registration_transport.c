#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "registration_api.h"

uint32_t Kh02UE4SSRegistrationCall(Kh02RegistrationRequest *request) {
    wchar_t path[1024];
    DWORD n = GetModuleFileNameW(NULL, path, 1024), end;
    static const wchar_t suffix[] = L"Mods\\KH02ReceiptHost\\dlls\\main.dll";
    HMODULE module;
    Kh02RegistrationGetApi query;
    Kh02RegistrationApi api = {0};
    if (!request || request->size != sizeof(*request) || request->version != 1 ||
        request->entered || request->fault || request->busy || request->reserved ||
        !request->permit || request->permit(request->owner_context) != 1 || !n || n >= 1024)
        return 0;
    end = n;
    while (end && path[end - 1] != L'\\' && path[end - 1] != L'/')
        --end;
    if (!end || end + sizeof(suffix) / sizeof(*suffix) > 1024)
        return 0;
    memcpy(path + end, suffix, sizeof(suffix));
    // Exact already-loaded sidecar only. No loader, remote thread, or fallback.
    module = GetModuleHandleW(path);
    if (!module)
        return 0;
    query =
        (Kh02RegistrationGetApi)(uintptr_t)GetProcAddress(module, "Kh02UE4SSGetRegistrationApiV1");
    if (!query || query(1, sizeof(api), &api) != 1 || api.size != sizeof(api) || api.version != 1 ||
        api.request_size != sizeof(*request) || api.reserved || !api.invoke)
        return 0;
    return api.invoke(request);
}
