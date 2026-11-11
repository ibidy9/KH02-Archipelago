#pragma once
#include "sdk_core_loader.hpp"
#include "sdk_pump_api.h"
#include "approved_core_hashes.hpp"
#include "dispatch_log.hpp"

namespace {
std::atomic<DWORD> sdk_owner_thread{0};
std::atomic_flag sdk_pumping = ATOMIC_FLAG_INIT;
Kh02SdkPumpFn sdk_pump{};
bool sdk_bound{}, sdk_blocked{};
std::uint64_t sdk_last_poll{};
kh02_sdk::DispatchLog sdk_log;
std::wstring sdk_core_path;

bool resolve_sdk_core() {
    if (sdk_blocked)
        return false;
    if (sdk_pump)
        return true;
    if (sdk_core_path.empty())
        sdk_core_path = kh02_sdk::runtime_core_path();
    const auto core = kh02_sdk::loaded_core(sdk_core_path, approved_core_hashes);
    if (core.state == kh02_sdk::CoreState::not_loaded)
        return false;
    if (core.state != kh02_sdk::CoreState::ready) {
        sdk_blocked = true;
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02OwnerDispatch] CORE_REFUSED path_hash_or_api\n"));
        return false;
    }
    sdk_pump = core.pump;
    RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02OwnerDispatch] CORE_VERIFIED already_loaded=true\n"));
    return true;
}

#include "mines_access.hpp"

void sdk_owner_post(ue::UObject *, ue::UFunction *, void *) {
    // One framework callback; no Lua task allocation or scan on each request.
    try {
        if (!active || !engine_ready)
            return;
        const DWORD current = GetCurrentThreadId();
        DWORD owner = sdk_owner_thread.load(std::memory_order_relaxed);
        if (!owner) {
            if (!bridge_thread())
                return;
            sdk_owner_thread.store(current, std::memory_order_relaxed);
            owner = current;
        }
        if (current != owner || sdk_blocked)
            return;
        kh02_host::ObservationLease lease(sdk_pumping);
        if (!lease)
            return;
        const auto now = GetTickCount64();
        if (now - sdk_last_poll < 10)
            return;
        sdk_last_poll = now;
        if (!sdk_bound) {
            PlainHint hint;
            if (!hint_copy(hint) || !hint.weak_ready)
                return;
        }
        if (!resolve_sdk_core())
            return;
        Kh02SdkPumpStatus status{};
        if (sdk_pump(KH02_SDK_PUMP_VERSION, sizeof(status), &status) != 1)
            return;
        if (status.size != sizeof(status) || status.version != KH02_SDK_PUMP_VERSION ||
            status.pid != GetCurrentProcessId() || status.thread != current || status.bound != 1) {
            sdk_blocked = true;
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02OwnerDispatch] PUMP_REFUSED identity_or_abi\n"));
            return;
        }
        if (sdk_log.observe(status, now)) {
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02OwnerDispatch] SERVICE bound=true sequence={} sdk_commands={} bootstrap_commands={} ignored_wakes={} status={}\n"),
                status.sequence, status.sdk_commands, status.window_commands, status.ignored_wakes,
                status.core_status);
        }
        sdk_bound = true;
        mines_reconcile(sdk_core_path);
    } catch (const std::exception &error) {
        sdk_blocked = true;
        bridge_exception(L"sdk_owner_dispatch", 0, error.what());
    }
}
} // namespace
