/* Included only in the experimental copied transport, after fresh_service. */
MEMBER_API uint32_t Kh02SdkOwnerPump(uint32_t version, uint32_t size, Kh02SdkPumpStatus *out) {
    uint32_t before, thread = GetCurrentThreadId();
    if (!out || version != KH02_SDK_PUMP_VERSION || size != sizeof(*out))
        return 0;
    memset(out, 0, sizeof(*out));
    if (!fresh_map || !fresh_cookie || thread != fresh_thread ||
        InterlockedCompareExchange(&fresh_servicing, 0, 0))
        return 0;
    if (!sdk_dispatch.bound) {
        if (!Kh02SdkCanBind(thread, fresh_thread, sdk_setup_ok, 0,
                            harness_router &&
                                !InterlockedCompareExchange(&harness_router->closed, 0, 0)))
            return 0;
        sdk_dispatch.bound = 1;
    }
    before = fresh_served;
    fresh_service(fresh_map);
    if (fresh_served != before)
        ++sdk_dispatch.sdk_commands;
    *out = (Kh02SdkPumpStatus){sizeof(*out),
                               KH02_SDK_PUMP_VERSION,
                               GetCurrentProcessId(),
                               thread,
                               sdk_dispatch.bound,
                               fresh_served,
                               sdk_dispatch.sdk_commands,
                               sdk_dispatch.window_commands,
                               fresh_map->transport.request_sequence > fresh_served,
                               fresh_map->transport.status,
                               (uint32_t)InterlockedCompareExchange(&fresh_servicing, 0, 0),
                               sdk_dispatch.ignored_wakes};
    return 1;
}
