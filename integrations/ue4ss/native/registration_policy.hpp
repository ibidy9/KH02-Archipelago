#pragma once
#include "registration_api.h"
#include <exception>

namespace kh02_registration {
// Closed catalog: no default pattern, arbitrary names, quantities or player IDs.
inline bool allowed(uint32_t code) {
    return (code >= 0x600 && code <= 0x633 && code != 0x62d) || (code >= 0xc00 && code <= 0xc03);
}

inline bool configured(const Kh02RegistrationRequest *p) {
    return p && p->size == sizeof(*p) && p->version == 1 && p->pawn && p->context &&
           allowed(p->code) && p->show == 1 && p->permit && p->owner_context && !p->entered &&
           !p->fault && !p->busy && !p->reserved;
}

struct Busy {
    Kh02RegistrationRequest &p;
    int exceptions{std::uncaught_exceptions()};

    explicit Busy(Kh02RegistrationRequest &request) : p(request) {
        p.busy = 1;
    }

    ~Busy() {
        if (std::uncaught_exceptions() > exceptions)
            p.fault = 1;
        p.busy = 0;
    }
};

template <class Backend> uint32_t invoke(Kh02RegistrationRequest *p, Backend &backend) {
    if (!configured(p))
        return 0;
    const auto original = *p;
    auto unchanged = [&] {
        return p->size == original.size && p->version == original.version &&
               p->pawn == original.pawn && p->context == original.context &&
               p->code == original.code && p->show == original.show &&
               p->permit == original.permit && p->owner_context == original.owner_context &&
               !p->reserved;
    };
    Busy busy(*p);
    if (!backend.prepare(*p))
        return 0;
    const auto revision = p->revision;
    // Revalidate every mutable field after external read/permit callbacks.
    if (!unchanged() || p->entered || p->fault || p->busy != 1 || !backend.validate(*p) ||
        !unchanged() || p->revision != revision || p->entered || p->fault || p->busy != 1)
        return 0;
    p->entered = 1;
    if (!backend.grant(*p)) {
        p->fault = 1;
        return 0;
    }
    if (!unchanged() || p->revision != revision || p->entered != 1 || p->fault || p->busy != 1 ||
        !backend.validate(*p) || !unchanged() || p->revision != revision || p->entered != 1 ||
        p->fault || p->busy != 1) {
        p->fault = 1;
        return 0;
    }
    return 1; // Not a receipt acknowledgement.
}
} // namespace kh02_registration
