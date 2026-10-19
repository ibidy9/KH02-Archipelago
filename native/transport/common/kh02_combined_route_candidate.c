#include "kh02_combined_route_candidate.h"
#include <stddef.h>
#include <string.h>

#ifndef KH02_ENABLE_COMBINED_ROUTE_HOOK
#define KH02_ENABLE_COMBINED_ROUTE_HOOK 0
#endif

_Static_assert(sizeof(Kh02RouteRule) == 48, "rule ABI");
_Static_assert(sizeof(Kh02RoutePolicy) == 408, "policy ABI");
_Static_assert(sizeof(Kh02RouteRequest) == 40, "request ABI");
_Static_assert(sizeof(Kh02RouteResult) == 24, "result ABI");
_Static_assert(sizeof(Kh02CleanupReceipt) == 48, "cleanup ABI");
_Static_assert(sizeof(Kh02RouteBlock) == 496, "block ABI");

uint32_t Kh02CombinedRouteLiveEnabled(void) { return KH02_ENABLE_COMBINED_ROUTE_HOOK; }

uint32_t Kh02ValidateRoutePolicy(const Kh02RoutePolicy *p) {
    uint32_t i, j, caller;
    if (!p || !p->owner_cookie || !p->generation || p->reserved || p->count > KH02_ROUTE_MAX_RULES)
        return KH02_INVALID;
    for (i = 0; i < p->count; ++i) {
        const Kh02RouteRule *r = &p->rules[i];
        switch (r->kind) {
            case KH02_START: case KH02_WORLD_WITHIN_EXIT: case KH02_FOREST_EXIT:
                caller = 0x43ECA3u; break;
            case KH02_FOREST_POSTGAME: caller = 0x313644u; break;
            case KH02_DEMON_TIDE: caller = 0x563C95u; break;
            default: return KH02_INVALID;
        }
        if (r->caller_rva != caller || r->enabled > 1 || r->once > 1
            || r->once != (r->kind == KH02_START)
            || !r->source_map || !r->source_spawn || !r->target_map || !r->target_spawn)
            return KH02_INVALID;
        /* No ordering-dependent first-match policy, even for disabled rows. */
        for (j = 0; j < i; ++j) {
            const Kh02RouteRule *a = &p->rules[j];
            if (a->caller_rva == r->caller_rva && a->source_map == r->source_map
                && a->source_spawn == r->source_spawn) return KH02_INVALID;
        }
    }
    return KH02_ALLOW;
}

uint32_t Kh02DecideRoute(const Kh02RoutePolicy *p, const Kh02RouteRequest *q, Kh02RouteResult *out) {
    uint32_t i;
    if (!q || !out) return KH02_INVALID;
    out->map = q->map; out->spawn = q->spawn; out->fade = q->fade; out->rule_index = UINT32_MAX;
    if (Kh02ValidateRoutePolicy(p) != KH02_ALLOW) return KH02_INVALID;
    if (p->owner_cookie != q->owner_cookie || p->generation != q->generation) return KH02_STALE;
    for (i = 0; i < p->count; ++i) {
        const Kh02RouteRule *r = &p->rules[i];
        if (r->enabled && r->caller_rva == q->caller_rva
            && r->source_map == q->map && r->source_spawn == q->spawn) {
            out->map = r->target_map; out->spawn = r->target_spawn; out->rule_index = i;
            return KH02_REDIRECT;
        }
    }
    return KH02_ALLOW;
}

uint32_t Kh02CleanupVerified(const Kh02CleanupReceipt *r, uint64_t cookie,
                            uint64_t generation, uint32_t sequence) {
    return r && cookie && generation && sequence && r->magic == KH02_ROUTE_MAGIC
        && r->version == KH02_ROUTE_VERSION && r->size == sizeof(*r)
        && r->owner_cookie == cookie && r->generation == generation
        && r->sequence == sequence && r->status == KH02_OK
        && r->active == 0 && r->entry_restored == 1 && r->in_flight == 0;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#if KH02_ENABLE_COMBINED_ROUTE_HOOK
/* Live bridge is compile-checked separately but never enabled in the shipped
   candidate DLL. Controller identity/FName/phase acquisition is not yet ready. */
typedef uint8_t (__fastcall *RouteFn)(void *, uint64_t, uint64_t, uint32_t);
static const unsigned char prefix[KH02_ROUTE_PATCH_SIZE] = {
    0x4C,0x89,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x55,0x53,0x57,0x48,0x8D,0x6C,0x24,0xC0
};
static unsigned char *image, *target, *trampoline;
static unsigned char owned_patch[KH02_ROUTE_PATCH_SIZE];
static RouteFn original_route;
static Kh02RoutePolicy current;
static volatile LONG active, in_flight, wrong_thread, matched, refused;
static DWORD owner_thread, original_protection;
static int restored = 1;

static void jump(unsigned char *p, const void *to) {
    uint64_t address = (uint64_t)(uintptr_t)to;
    p[0] = 0xFF; p[1] = 0x25; memset(p + 2, 0, 4); memcpy(p + 6, &address, 8);
}

static uint8_t __fastcall route_hook(void *gi, uint64_t map, uint64_t spawn, uint32_t fade) {
    Kh02RouteRequest q; Kh02RouteResult result; uint8_t returned;
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    InterlockedIncrement(&in_flight);
    result.map = map; result.spawn = spawn; result.fade = fade;
    if (GetCurrentThreadId() != owner_thread) InterlockedExchange(&wrong_thread, 1);
    else if (InterlockedCompareExchange(&active, 0, 0)) {
        q.owner_cookie = current.owner_cookie; q.generation = current.generation;
        q.map = map; q.spawn = spawn; q.fade = fade;
        q.caller_rva = caller >= (uintptr_t)image && caller - (uintptr_t)image < 0x04F5F000u
            ? (uint32_t)(caller - (uintptr_t)image) : UINT32_MAX;
        if (Kh02DecideRoute(&current, &q, &result) == KH02_REDIRECT) {
            InterlockedIncrement(&matched);
            if (current.rules[result.rule_index].once) current.rules[result.rule_index].enabled = 0;
        } else InterlockedIncrement(&refused);
    }
    returned = original_route(gi, result.map, result.spawn, result.fade);
    InterlockedDecrement(&in_flight);
    return returned;
}

static int exact_image(void) {
    IMAGE_DOS_HEADER *dos; IMAGE_NT_HEADERS64 *nt;
    static const unsigned char shared_call[5] = {0xE8,0x9D,0xA2,0x01,0x00};
    static const unsigned char demon_call[5] = {0xE8,0xAB,0x52,0xEF,0xFF};
    static const unsigned char forest_call[5] = {0x8B,0xCA,0xFF,0x50,0x20};
    image = (unsigned char *)GetModuleHandleW(NULL);
    if (!image) return 0;
    dos = (IMAGE_DOS_HEADER *)image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 0x100000) return 0;
    nt = (IMAGE_NT_HEADERS64 *)(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage != 0x04F5F000u) return 0;
    target = image + KH02_ROUTE_RVA;
    return !memcmp(target, prefix, sizeof(prefix))
        && !memcmp(image + 0x43EC9Eu, shared_call, sizeof(shared_call))
        && !memcmp(image + 0x563C90u, demon_call, sizeof(demon_call))
        && !memcmp(image + 0x31363Fu, forest_call, sizeof(forest_call));
}

static uint32_t arm(const Kh02RoutePolicy *policy) {
    DWORD old, ignored; HMODULE pinned;
    if (Kh02ValidateRoutePolicy(policy) != KH02_ALLOW) return KH02_BAD_REQUEST;
    if (active || !restored || in_flight || wrong_thread) return KH02_BUSY;
    if (!exact_image()) return KH02_BAD_BUILD;
    /* Deliberately retain this module and ONE trampoline until process exit.
       A branch already entering the old hook must never land in freed code. */
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           (LPCWSTR)(uintptr_t)route_hook, &pinned)) return KH02_CLEANUP_UNCERTAIN;
    if (!trampoline) {
        trampoline = VirtualAlloc(NULL, KH02_ROUTE_PATCH_SIZE + 14, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!trampoline) return KH02_CLEANUP_UNCERTAIN;
        memcpy(trampoline, prefix, sizeof(prefix)); jump(trampoline + sizeof(prefix), target + sizeof(prefix));
        if (!FlushInstructionCache(GetCurrentProcess(), trampoline, sizeof(prefix) + 14)) return KH02_CLEANUP_UNCERTAIN;
        original_route = (RouteFn)trampoline;
    }
    memset(owned_patch, 0x90, sizeof(owned_patch)); jump(owned_patch, route_hook);
    if (!VirtualProtect(target, sizeof(prefix), PAGE_EXECUTE_READWRITE, &old)) return KH02_CLEANUP_UNCERTAIN;
    original_protection = old; current = *policy; owner_thread = GetCurrentThreadId(); restored = 0;
    memcpy(target, owned_patch, sizeof(owned_patch));
    if (!FlushInstructionCache(GetCurrentProcess(), target, sizeof(prefix))) return KH02_CLEANUP_UNCERTAIN;
    if (!VirtualProtect(target, sizeof(prefix), old, &ignored)) return KH02_CLEANUP_UNCERTAIN;
    if (memcmp(target, owned_patch, sizeof(owned_patch))) return KH02_PATCH_CONFLICT;
    InterlockedExchange(&active, 1);
    return KH02_OK;
}

static uint32_t disarm(void) {
    DWORD ignored;
    InterlockedExchange(&active, 0);  /* Stop applying policy while retaining code ownership. */
    if (in_flight || wrong_thread) return KH02_BUSY;  /* Never block the callback thread. */
    if (restored) return KH02_OK;
    if (memcmp(target, owned_patch, sizeof(owned_patch)) && memcmp(target, prefix, sizeof(prefix)))
        return KH02_PATCH_CONFLICT;  /* Never overwrite somebody else's detour. */
    if (!VirtualProtect(target, sizeof(prefix), PAGE_EXECUTE_READWRITE, &ignored)) return KH02_CLEANUP_UNCERTAIN;
    memcpy(target, prefix, sizeof(prefix));
    if (!FlushInstructionCache(GetCurrentProcess(), target, sizeof(prefix))) return KH02_CLEANUP_UNCERTAIN;
    if (!VirtualProtect(target, sizeof(prefix), original_protection, &ignored)) return KH02_CLEANUP_UNCERTAIN;
    if (memcmp(target, prefix, sizeof(prefix))) return KH02_CLEANUP_UNCERTAIN;
    restored = 1;
    return KH02_OK;
}
#endif

static void service(void) {
    wchar_t name[96]; HANDLE mapping; Kh02RouteBlock *b; uint32_t seq;
    _snwprintf_s(name, 96, _TRUNCATE, L"Local\\KH02APCombinedRouteCandidate-%lu", GetCurrentProcessId());
    mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (!mapping) return;
    b = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*b));
    if (!b) { CloseHandle(mapping); return; }
    seq = b->request_sequence;
    if (b->magic != KH02_ROUTE_MAGIC || b->version != KH02_ROUTE_VERSION || b->size != sizeof(*b)
        || !seq || seq <= b->response_sequence) goto done;
    b->status = KH02_DISABLED;
#if KH02_ENABLE_COMBINED_ROUTE_HOOK
    if (b->command == KH02_ARM) b->status = arm(&b->policy);
    else if (b->command == KH02_DISARM) {
        if (b->policy.owner_cookie != current.owner_cookie || b->policy.generation != current.generation)
            b->status = KH02_BAD_REQUEST;
        else b->status = disarm();
    } else b->status = KH02_BAD_REQUEST;
    b->active = (uint32_t)active;
    b->cleanup.entry_restored = (uint32_t)restored;
    b->cleanup.in_flight = (uint32_t)in_flight;
    b->matched = (uint32_t)matched; b->refused = (uint32_t)refused;
#else
    b->active = 0; b->cleanup.entry_restored = 0; b->cleanup.in_flight = 0;
#endif
    b->cleanup.magic = KH02_ROUTE_MAGIC; b->cleanup.version = KH02_ROUTE_VERSION;
    b->cleanup.size = sizeof(b->cleanup); b->cleanup.sequence = seq;
    b->cleanup.owner_cookie = b->policy.owner_cookie; b->cleanup.generation = b->policy.generation;
    b->cleanup.status = b->status; b->cleanup.active = b->active;
    MemoryBarrier(); b->response_sequence = seq;
done:
    UnmapViewOfFile(b); CloseHandle(mapping);
}

__declspec(dllexport) LRESULT CALLBACK Kh02CombinedRouteCandidateHook(int code, WPARAM w, LPARAM l) {
    /* WH_GETMESSAGE can observe PM_NOREMOVE too; service only removed messages. */
    if (code == HC_ACTION && w == PM_REMOVE && l && ((const MSG *)l)->message == KH02_ROUTE_MESSAGE) service();
    return CallNextHookEx(NULL, code, w, l);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(instance);
    return TRUE;
}
#endif
