#pragma once
#include "bridge_api.h"
#include <cstring>
#include <exception>

namespace kh02_bridge {
inline bool configured(const Kh02UE4SSRequest *p) {
    return p && p->size == sizeof(*p) && p->version == KH02_UE4SS_BRIDGE_VERSION && p->pawn &&
           p->permit && p->owner_context && !p->reserved && !p->reserved0 && !p->fault &&
           p->converted <= 1 && p->entered <= 1 && p->busy <= 1 &&
           (!p->converted || (p->name.index && p->battle <= 6)) && (!p->entered || p->converted);
}

inline bool zeros(const uint8_t (&p)[7]) {
    for (auto b : p)
        if (b)
            return false;
    return true;
}

template <typename Backend>
uint32_t check_request(Kh02UE4SSRequest *p, uint64_t *world, Backend &backend) {
    if (!world)
        return 0;
    *world = 0;
    if (!configured(p) || p->busy)
        return 0;
    uint64_t found = 0;
    if (!backend.resolve(*p, found) || !configured(p) || found != p->pawn)
        return 0;
    *world = found;
    return 1;
}

struct Busy {
    Kh02UE4SSRequest &request;
    int exceptions{std::uncaught_exceptions()};

    explicit Busy(Kh02UE4SSRequest &p) : request(p) {
        request.busy = 1;
    }

    ~Busy() {
        if (std::uncaught_exceptions() > exceptions)
            request.fault = 1;
        request.busy = 0;
    }
};

template <typename Backend>
uint32_t invoke(Kh02UE4SSRequest *p, uint32_t operation, void *data, uint32_t size,
                Backend &backend) {
    if (!data || size != 24 || (operation != 1 && operation != 2))
        return 0;
    uint64_t world = 0;
    if (!check_request(p, &world, backend))
        return 0;
    Busy busy(*p);
    if (operation == 1) {
        Kh02UE4SSConvert frame{};
        std::memcpy(&frame, data, sizeof(frame));
        if (frame.world != world || frame.battle > 6 || !zeros(frame.reserved) ||
            frame.result.index || frame.result.number || p->converted || p->entered)
            return 0;
        if (!backend.convert(*p, frame) || !frame.result.index || !configured(p))
            return 0;
        uint64_t again = 0;
        if (!backend.resolve(*p, again) || again != world || !configured(p))
            return 0;
        p->battle = frame.battle;
        p->name = frame.result;
        p->converted = 1;
        std::memcpy(data, &frame, sizeof(frame));
        return 1;
    }
    Kh02UE4SSGrant frame{};
    std::memcpy(&frame, data, sizeof(frame));
    if (frame.world != world || frame.show > 1 || !zeros(frame.reserved) || !p->converted ||
        p->entered || frame.name.index != p->name.index || frame.name.number != p->name.number)
        return 0;
    // Consume before entering the gameplay call, including a call that fails.
    p->entered = 1;
    if (!backend.grant(*p, frame)) {
        p->fault = 1;
        return 0;
    }
    uint64_t again = 0;
    if (!backend.resolve(*p, again) || again != world || !configured(p)) {
        p->fault = 1;
        return 0;
    }
    return 1; // Receipt acknowledgement still requires the existing inventory check.
}
} // namespace kh02_bridge
