#include "reflected_grant.h"
#include <stddef.h>
#include <string.h>
_Static_assert(sizeof(Kh02ReflectedConvertParams) == 24, "converter parameter size");
_Static_assert(offsetof(Kh02ReflectedConvertParams, battle_id) == 8, "battle offset");
_Static_assert(offsetof(Kh02ReflectedConvertParams, result) == 16, "return offset");
_Static_assert(sizeof(Kh02ReflectedGrantParams) == 24, "grant parameter size");
_Static_assert(offsetof(Kh02ReflectedGrantParams, name) == 8, "name offset");
_Static_assert(offsetof(Kh02ReflectedGrantParams, show) == 16, "show offset");

static uint32_t check(void *opaque) {
    Kh02ReflectedGrantAdapter *a = opaque;
    uint64_t world = 0;
    return a && !a->fault && a->check && a->invoke && a->delegate.check &&
           a->delegate.check(a->delegate.context) == 1 && a->check(a->bridge, &world) == 1 &&
           world && world == a->world_context;
}

static uint32_t read_sample(void *opaque, Kh02SessionSample *sample,
                            Kh02InventorySnapshot *inventory) {
    Kh02ReflectedGrantAdapter *a = opaque;
    return check(a) && a->delegate.read(a->delegate.context, sample, inventory) == 1 && check(a);
}

static uint32_t convert(void *opaque, uint32_t battle, Kh02ReceiptItemName *out) {
    Kh02ReflectedGrantAdapter *a = opaque;
    Kh02ReflectedConvertParams params = {0};
    if (!out)
        return 0;
    memset(out, 0, sizeof(*out));
    if (battle > 6 || !check(a))
        return 0;
    params.world_context = a->world_context;
    params.battle_id = (uint8_t)battle;
    if (a->invoke(a->bridge, 1, &params, sizeof(params)) != 1 || !check(a) || !params.result.index)
        return 0;
    *out = params.result;
    return 1;
}

static void grant(void *opaque, const Kh02ReceiptItemName *name, uint32_t show) {
    Kh02ReflectedGrantAdapter *a = opaque;
    Kh02ReflectedGrantParams params = {0};
    if (!name || !name->index || show > 1 || !check(a))
        return;
    params.world_context = a->world_context;
    params.name = *name;
    params.show = (uint8_t)show;
    /* Receipt core verifies inventory afterward. A successful Invoke is NOT a
       receipt acknowledgement; failure after entry must remain uncertain. */
    if (a->invoke(a->bridge, 2, &params, sizeof(params)) != 1)
        a->fault = 1;
}

uint32_t Kh02ReflectedGrantAttach(Kh02ReceiptGrant *packet, Kh02ReflectedGrantAdapter *adapter,
                                  Kh02ReflectedCheck bridge_check, Kh02ReflectedInvoke invoke,
                                  void *bridge) {
    Kh02ReflectedGrantAdapter candidate = {0};
    uint64_t world = 0;
    if (!packet || !adapter || packet->context == adapter || packet->check == check ||
        !packet->owner || !packet->session || !packet->check || !packet->read || !bridge_check ||
        !invoke || packet->check(packet->context) != 1 || bridge_check(bridge, &world) != 1 ||
        !world)
        return 0;
    candidate.delegate = *packet;
    candidate.check = bridge_check;
    candidate.invoke = invoke;
    candidate.bridge = bridge;
    candidate.world_context = world;
    *adapter = candidate;
    packet->check = check;
    packet->read = read_sample;
    packet->convert = convert;
    packet->grant = grant;
    packet->context = adapter;
    return 1;
}
