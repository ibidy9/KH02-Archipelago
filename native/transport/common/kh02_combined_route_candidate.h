#ifndef KH02_COMBINED_ROUTE_CANDIDATE_H
#define KH02_COMBINED_ROUTE_CANDIDATE_H
#include <stdint.h>

#ifdef _WIN32
#define KH02_EXPORT __declspec(dllexport)
#else
#define KH02_EXPORT
#endif

#define KH02_ROUTE_MAGIC 0x4B435254u
#define KH02_ROUTE_VERSION 1u
#define KH02_ROUTE_MAX_RULES 8u
#define KH02_ROUTE_MESSAGE 0x8260u
#define KH02_ROUTE_RVA 0x458F40u
#define KH02_ROUTE_PATCH_SIZE 18u

enum Kh02RouteKind {
    KH02_START = 1, KH02_WORLD_WITHIN_EXIT = 2, KH02_FOREST_EXIT = 3,
    KH02_FOREST_POSTGAME = 4, KH02_DEMON_TIDE = 5
};
enum Kh02Decision { KH02_ALLOW = 0, KH02_REDIRECT = 1, KH02_INVALID = 2, KH02_STALE = 3 };
enum Kh02Status {
    KH02_OK = 1, KH02_BAD_REQUEST = 2, KH02_BAD_BUILD = 3,
    KH02_DISABLED = 4, KH02_BUSY = 5, KH02_PATCH_CONFLICT = 6,
    KH02_CLEANUP_UNCERTAIN = 7
};
enum Kh02Command { KH02_ARM = 1, KH02_DISARM = 2 };

/* FNames are resolved for this process by the adapter, never saved IDs.
   Matching the shared caller alone is explicitly prohibited. */
typedef struct Kh02RouteRule {
    uint32_t kind, caller_rva, enabled, once;
    uint64_t source_map, source_spawn, target_map, target_spawn;
} Kh02RouteRule;

typedef struct Kh02RoutePolicy {
    uint64_t owner_cookie, generation;
    uint32_t count, reserved;
    Kh02RouteRule rules[KH02_ROUTE_MAX_RULES];
} Kh02RoutePolicy;

typedef struct Kh02RouteRequest {
    uint64_t owner_cookie, generation, map, spawn;
    uint32_t caller_rva, fade;
} Kh02RouteRequest;

typedef struct Kh02RouteResult {
    uint64_t map, spawn;
    uint32_t fade, rule_index;
} Kh02RouteResult;

typedef struct Kh02CleanupReceipt {
    uint32_t magic, version, size, sequence;
    uint64_t owner_cookie, generation;
    uint32_t status, active, entry_restored, in_flight;
} Kh02CleanupReceipt;

/* Requests/configuration are accepted only on the native game-window thread.
   The controller must wait for sequence acknowledgment before another request. */
typedef struct Kh02RouteBlock {
    uint32_t magic, version, size, request_sequence;
    uint32_t response_sequence, command, status, active;
    Kh02RoutePolicy policy;
    Kh02CleanupReceipt cleanup;
    uint32_t matched, refused;
} Kh02RouteBlock;

KH02_EXPORT uint32_t Kh02ValidateRoutePolicy(const Kh02RoutePolicy *policy);
KH02_EXPORT uint32_t Kh02DecideRoute(const Kh02RoutePolicy *policy,
    const Kh02RouteRequest *request, Kh02RouteResult *result);
KH02_EXPORT uint32_t Kh02CleanupVerified(const Kh02CleanupReceipt *receipt,
    uint64_t cookie, uint64_t generation, uint32_t sequence);
KH02_EXPORT uint32_t Kh02CombinedRouteLiveEnabled(void);
#endif
