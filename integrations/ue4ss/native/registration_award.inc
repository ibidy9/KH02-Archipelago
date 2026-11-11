/* Included after the unchanged Award/valid definitions in each generated TU. */
#include "registration_api.h"
uint32_t Kh02UE4SSRegistrationCall(Kh02RegistrationRequest *);

typedef struct RegistrationPermitContext {
    Award *award;
    Kh02RegistrationRequest *request;
} RegistrationPermitContext;

static uint32_t registration_permit(void *opaque) {
    RegistrationPermitContext *p = opaque;
    Kh02CosmeticSnapshot observed;
    if (!p || !p->award || !p->request || !valid(p->award, &observed))
        return 0;
    // Before entry, require the exact original snapshot; afterward leave the
    // original per-record verification to effect(), as in the proven path.
    return p->request->entered || !memcmp(&observed, &p->award->attempt->before, sizeof(observed));
}

static uint32_t registration_award(Award *a) {
    Kh02RegistrationRequest request = {0};
    RegistrationPermitContext permit = {a, &request};
    uint32_t result;
    request.size = sizeof(request);
    request.version = 1;
    request.pawn = a->lease.provider;
    request.context = a->game->session->context;
    request.code = a->descriptor.code;
    request.show = 1;
    request.permit = registration_permit;
    request.owner_context = &permit;
    result = Kh02UE4SSRegistrationCall(&request);
    a->attempt->entered = request.entered;
    return result == 1 && request.entered == 1 && !request.fault;
}
