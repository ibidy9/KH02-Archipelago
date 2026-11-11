#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "transport_bridge.h"

static uint32_t attach_api(Kh02ReceiptGrant *packet, uint64_t pawn,
                           Kh02ReflectedGrantAdapter *adapter, Kh02UE4SSRequest *request,
                           const Kh02UE4SSApi *api) {
    Kh02ReceiptGrant candidate;
    if (!packet || !adapter || !request || !api || api->size != sizeof(*api) ||
        api->version != KH02_UE4SS_BRIDGE_VERSION || api->request_size != sizeof(*request) ||
        api->features != KH02_UE4SS_BRIDGE_FEATURES || !api->check || !api->invoke ||
        !packet->owner || !packet->session || !packet->check || !packet->context || !pawn ||
        pawn != packet->lease.provider || pawn != packet->session->gate.current.provider)
        return 0;
    memset(request, 0, sizeof(*request));
    request->size = sizeof(*request);
    request->version = KH02_UE4SS_BRIDGE_VERSION;
    request->pawn = pawn;
    request->permit = packet->check;
    request->owner_context = packet->context;
    candidate = *packet;
    if (Kh02ReflectedGrantAttach(&candidate, adapter, api->check, api->invoke, request) != 1) {
        memset(request, 0, sizeof(*request));
        return 0;
    }
    *packet = candidate;
    return 1;
}

uint32_t Kh02UE4SSTransportAttach(Kh02ReceiptGrant *packet, uint64_t pawn,
                                  Kh02ReflectedGrantAdapter *adapter, Kh02UE4SSRequest *request) {
    wchar_t path[1024];
    DWORD length = GetModuleFileNameW(NULL, path, 1024), end;
    static const wchar_t suffix[] = L"Mods\\KH02ReceiptHost\\dlls\\main.dll";
    Kh02UE4SSApi api = {0};
    HMODULE module;
    Kh02UE4SSGetApi query;
    if (!length || length >= 1024)
        return 0;
    end = length;
    while (end && path[end - 1] != L'\\' && path[end - 1] != L'/')
        --end;
    if (!end || end + sizeof(suffix) / sizeof(*suffix) > 1024)
        return 0;
    memcpy(path + end, suffix, sizeof(suffix));
    // Only query the exact already-loaded sidecar. Never load/search for a DLL.
    module = GetModuleHandleW(path);
    if (!module)
        return 0;
    query = (Kh02UE4SSGetApi)(uintptr_t)GetProcAddress(module, "Kh02UE4SSGetApiV1");
    if (!query || query(KH02_UE4SS_BRIDGE_VERSION, sizeof(api), &api) != 1)
        return 0;
    return attach_api(packet, pawn, adapter, request, &api);
}
#ifdef KH02_UE4SS_BRIDGE_SELFTEST
__declspec(dllexport) uint32_t Kh02UE4SSTestLoadedQuery(void) {
    return Kh02UE4SSTransportAttach(NULL, 0, NULL, NULL);
}

__declspec(dllexport) uint32_t Kh02UE4SSTestAttach(Kh02ReceiptGrant *packet, uint64_t pawn,
                                                   Kh02ReflectedGrantAdapter *adapter,
                                                   Kh02UE4SSRequest *request,
                                                   const Kh02UE4SSApi *api) {
    return attach_api(packet, pawn, adapter, request, api);
}
#endif
