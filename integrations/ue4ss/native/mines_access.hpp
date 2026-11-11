#pragma once
#include "mines_notice.hpp"

// Called only from the existing authenticated SDK owner pump, on its game thread.
// Ownership comes from the same branch history as the proven movement gate.
namespace {
using MinesOwnershipFn = uint32_t (*)(uint32_t, uint32_t, uint32_t *);
MinesOwnershipFn mines_ownership{};
ue::FWeakObjectPtr mines_held;
uint64_t mines_last_poll{};
bool mines_refused{};

void mines_reconcile(const std::wstring &core_path) {
    if (mines_refused)
        return;
    const auto now = GetTickCount64();
    if (now - mines_last_poll < 100)
        return;
    mines_last_poll = now;
    if (!mines_ownership) {
        // Caller has already checked the loaded module's exact path, hash and owner ABI.
        const auto address =
            GetProcAddress(GetModuleHandleW(core_path.c_str()), "Kh02SdkMovementOwnershipV1");
        if (!address) {
            mines_refused = true;
            return;
        }
        static_assert(sizeof(address) == sizeof(mines_ownership));
        std::memcpy(&mines_ownership, &address, sizeof(address));
    }
    uint32_t mask{};
    if (mines_ownership(1, sizeof(mask), &mask) != 1)
        return;
    PlainHint hint;
    if (!hint_copy(hint) || !hint.weak_ready)
        return;
    auto *pawn = hinted_instance(hint, false);
    if (!pawn)
        return;
    const auto pawn_path = pawn->GetFullName();
    if (pawn_path.find(L"/Game/Levels/dw/dw_03/dw_03.") == std::wstring::npos) {
        mines_held = ue::FWeakObjectPtr{};
        mines_notice_policy.leave();
        return;
    }
    constexpr auto actor_path =
        L"/Game/Levels/dw/dw_03/umap/dw_03_gimmick.dw_03_gimmick:PersistentLevel.g_dw_Mirror_Tunnel_NEW";
    auto *actor =
        ue::UObjectGlobals::StaticFindObject(ue::UObject::StaticClass(), nullptr, actor_path);
    if (!live(actor) || !live(actor->GetClassPrivate()) ||
        actor->GetClassPrivate()->GetNamePrivate().ToString() != L"g_dw_MirrorA_C")
        return;
    auto *state = actor->GetClassPrivate()->FindProperty(ue::FName(L"temp_State"));
    if (!state || state->GetClass().GetFName().ToString() != L"ByteProperty" ||
        state->GetOffset_Internal() < 0 || state->GetOffset_Internal() > 0x4000) {
        mines_refused = true;
        return;
    }
    // State 0 is the intact, interactive mirror. State 1 is shattered.
    // Never reopen a consumed mirror, even if it was disabled by AP beforehand.
    if (*state->ContainerPtrToValuePtr<uint8_t>(actor) != 0) {
        mines_held = ue::FWeakObjectPtr{};
        mines_notice_policy.leave();
        return;
    }
    const auto component_path = std::wstring(actor_path) + L".TresReactor";
    auto *component = ue::UObjectGlobals::StaticFindObject(ue::UObject::StaticClass(), nullptr,
                                                           component_path.c_str());
    if (!live(component) || !live(component->GetClassPrivate()) ||
        component->GetClassPrivate()->GetNamePrivate().ToString() != L"TresReactorComponent")
        return;
    auto *query = function(L"/Script/TresGame.TresGimmickComponentBase:BP_IsGimmickComponentActive",
                           0x54020400, 1, 1, 0, {{L"ReturnValue", 0, L"BoolProperty"}});
    if (!query) {
        mines_refused = true;
        return;
    }
    uint8_t enabled = 0;
    component->ProcessEvent(query, &enabled);
    const bool owned = (mask & 1) != 0, held = mines_held.Get() == component;
    if (owned && enabled) {
        mines_held = ue::FWeakObjectPtr{};
        mines_notice_policy.leave();
        return;
    }
    if (owned && !held)
        return;
    if (!owned && !enabled) {
        if (held)
            mines_notice(pawn, actor, now);
        return;
    }
    const auto *path =
        owned ? L"/Script/TresGame.TresGimmickComponentBase:BP_ActivateGimmickComponent"
              : L"/Script/TresGame.TresGimmickComponentBase:BP_DeactivateGimmickComponent";
    auto *change = function(path, 0x4020400, 0, 0, 0xffff, {});
    uint32_t again{};
    PlainHint current;
    if (!change || !hint_copy(current) || current.revision != hint.revision ||
        mines_ownership(1, sizeof(again), &again) != 1 || again != mask)
        return;
    component->ProcessEvent(change, nullptr);
    uint8_t after = 0;
    component->ProcessEvent(query, &after);
    if ((after != 0) != owned) {
        mines_refused = true;
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02MinesAccess] REFUSED activation_readback\n"));
        return;
    }
    if (owned)
        mines_held = ue::FWeakObjectPtr{};
    else
        mines_held = component;
    RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02MinesAccess] APPLIED doubleflight={} prompt_enabled={}\n"), owned, after != 0);
}
} // namespace
