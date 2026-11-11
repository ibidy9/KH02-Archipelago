#ifndef KH02_UE4SS_REGISTRATION_API_H
#define KH02_UE4SS_REGISTRATION_API_H
#include <stdint.h>
/* Trusted synchronous owner-thread ABI, never a mapped/network request.
 * Ownership snapshots and durable receipt acknowledgement remain in the C core.
 * A request is consumed before ProcessEvent, even when that call fails. */
typedef uint32_t (*Kh02RegistrationPermit)(void *);

typedef struct Kh02RegistrationRequest {
    uint32_t size, version;
    uint64_t pawn, context;
    uint32_t code, show;
    Kh02RegistrationPermit permit;
    void *owner_context;
    uint64_t revision;
    uint32_t entered, fault, busy, reserved;
} Kh02RegistrationRequest;

typedef uint32_t (*Kh02RegistrationInvoke)(Kh02RegistrationRequest *);

typedef struct Kh02RegistrationApi {
    uint32_t size, version, request_size, reserved;
    Kh02RegistrationInvoke invoke;
} Kh02RegistrationApi;

typedef uint32_t (*Kh02RegistrationGetApi)(uint32_t, uint32_t, Kh02RegistrationApi *);
#if defined(__cplusplus)
static_assert(sizeof(Kh02RegistrationRequest) == 72 && sizeof(Kh02RegistrationApi) == 24);
#else
_Static_assert(sizeof(Kh02RegistrationRequest) == 72 && sizeof(Kh02RegistrationApi) == 24,
               "registration ABI");
#endif
#endif
