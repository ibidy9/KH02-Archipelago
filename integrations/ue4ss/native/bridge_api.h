#ifndef KH02_UE4SS_BRIDGE_API_H
#define KH02_UE4SS_BRIDGE_API_H
#include <stdint.h>
#include <stddef.h>
#define KH02_UE4SS_BRIDGE_VERSION 1u
#define KH02_UE4SS_BRIDGE_FEATURES 7u
/* Owned, synchronous request, never a shared-memory/network packet. The owner
   callback/context must remain valid until the receipt operation has returned.
   Integer identities are compared with freshly resolved objects, not cast. */
typedef uint32_t (*Kh02UE4SSPermit)(void *);

typedef struct Kh02UE4SSName {
    uint32_t index, number;
} Kh02UE4SSName;

typedef struct Kh02UE4SSRequest {
    uint32_t size, version;
    uint64_t pawn;
    Kh02UE4SSPermit permit;
    void *owner_context;
    uint64_t revision;
    uint32_t converted, entered, fault, battle, busy, reserved0;
    Kh02UE4SSName name;
    uint64_t reserved;
} Kh02UE4SSRequest;

typedef uint32_t (*Kh02UE4SSCheck)(void *, uint64_t *);
typedef uint32_t (*Kh02UE4SSInvoke)(void *, uint32_t, void *, uint32_t);

typedef struct Kh02UE4SSApi {
    uint32_t size, version, request_size, features;
    Kh02UE4SSCheck check;
    Kh02UE4SSInvoke invoke;
} Kh02UE4SSApi;

typedef uint32_t (*Kh02UE4SSGetApi)(uint32_t, uint32_t, Kh02UE4SSApi *);

typedef struct Kh02UE4SSConvert {
    uint64_t world;
    uint8_t battle, reserved[7];
    Kh02UE4SSName result;
} Kh02UE4SSConvert;

typedef struct Kh02UE4SSGrant {
    uint64_t world;
    Kh02UE4SSName name;
    uint8_t show, reserved[7];
} Kh02UE4SSGrant;
#if defined(__cplusplus)
static_assert(sizeof(Kh02UE4SSRequest) == 80 && sizeof(Kh02UE4SSApi) == 32);
static_assert(sizeof(Kh02UE4SSConvert) == 24 && offsetof(Kh02UE4SSConvert, result) == 16);
static_assert(sizeof(Kh02UE4SSGrant) == 24 && offsetof(Kh02UE4SSGrant, show) == 16);
#else
_Static_assert(sizeof(Kh02UE4SSRequest) == 80 && sizeof(Kh02UE4SSApi) == 32, "host ABI");
_Static_assert(sizeof(Kh02UE4SSConvert) == 24 && sizeof(Kh02UE4SSGrant) == 24, "parameter ABI");
#endif
#endif
