#pragma once

namespace {
MinesNoticePolicy mines_notice_policy;
bool mines_notice_refused{}, mines_notice_contract_logged{};

// Borrowed, immutable input strings, kept alive for the entire module lifetime.
// TresUI_OpenInformation takes a const TresLocText reference and resolves/copies
// it synchronously. These descriptors are not engine-owned or native-freed.
struct MinesString {
    const wchar_t *data{};
    int32_t count{}, capacity{};
};

struct MinesInformation {
    MinesString name_space{}, key{};

    struct {
        void *data{};
        int32_t count{}, capacity{};
    } params;

    MinesString text{};
    uint8_t auto_close{1}, pad[7]{};
};

static_assert(sizeof(MinesInformation) == 72 && offsetof(MinesInformation, text) == 48);

bool mines_text_contract(ue::UFunction *fn) {
    auto *field = fn->FindProperty(ue::FName(L"InformationText"));
    auto *type = static_cast<ue::FStructProperty *>(field)->GetStruct();
    if (!live(type) || type->GetFullName() != L"ScriptStruct /Script/TresGame.TresLocText" ||
        field->GetSize() != 64)
        return false;

    const struct {
        const wchar_t *name;
        int offset;
        const wchar_t *kind;
    } fields[] = {{L"Namespace", 0, L"StrProperty"},
                  {L"Key", 16, L"StrProperty"},
                  {L"Params", 32, L"ArrayProperty"},
                  {L"LocalizedText", 48, L"StrProperty"}};

    for (const auto &f : fields) {
        auto *p = type->FindProperty(ue::FName(f.name));
        if (!p || p->GetOffset_Internal() != f.offset || p->GetSize() != 16 ||
            p->GetClass().GetFName().ToString() != f.kind)
            return false;
    }
    return true;
}

void mines_notice(ue::UObject *pawn, ue::UObject *mirror, uint64_t now) {
    if (mines_notice_refused)
        return;
    auto *distance =
        function(L"/Script/Engine.Actor:GetDistanceTo", 0x54020401, 2, 12, 8,
                 {{L"OtherActor", 0, L"ObjectProperty"}, {L"ReturnValue", 8, L"FloatProperty"}});
    if (!distance) {
        mines_notice_refused = true;
        return;
    }
    // UFunction::ParmsSize describes the reflected parameter extent, not the
    // compiler's padded sizeof(struct). Check the actual extent against our
    // backing buffer, then independently validate every field below.
    auto *raw_show = exact(L"/Script/TresGame.TresGameBlueprintLibrary:TresUI_OpenInformation");
    if (!raw_show || raw_show->GetClassPrivate() != ue::UFunction::StaticClass()) {
        mines_notice_refused = true;
        return;
    }
    auto *show_fn = static_cast<ue::UFunction *>(raw_show);
    const auto extent = show_fn->GetParmsSize();
    if (extent < offsetof(MinesInformation, auto_close) + sizeof(uint8_t) ||
        extent > sizeof(MinesInformation)) {
        mines_notice_refused = true;
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02MinesNotice] REFUSED information_extent={} buffer={}\n"), extent,
            sizeof(MinesInformation));
        return;
    }
    auto *show = function(
        L"/Script/TresGame.TresGameBlueprintLibrary:TresUI_OpenInformation", 0x4422401, 2, extent,
        0xffff, {{L"InformationText", 0, L"StructProperty"}, {L"AutoClose", 64, L"BoolProperty"}});
    auto *target = exact(L"/Script/TresGame.Default__TresGameBlueprintLibrary");
    if (!show || !target || !mines_text_contract(show)) {
        mines_notice_refused = true;
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02MinesNotice] REFUSED information_contract; entry gate unchanged\n"));
        return;
    }
    if (!mines_notice_contract_logged) {
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02MinesNotice] CONTRACT_READY distance_extent=12 information_extent={} buffer={}\n"),
            extent, sizeof(MinesInformation));
        mines_notice_contract_logged = true;
    }

    struct {
        ue::UObject *actor;
        float result;
        uint32_t pad;
    } args{mirror, -1, 0};

    static_assert(offsetof(decltype(args), result) == 8 && sizeof(args) >= 12);
    pawn->ProcessEvent(distance, &args);
    if (!mines_notice_policy.ready(args.result, now))
        return;
    static constexpr wchar_t text[] = L"Doubleflight required";
    static MinesInformation info{
        {}, {}, {}, {text, sizeof(text) / sizeof(wchar_t), sizeof(text) / sizeof(wchar_t)}, 1, {}};
    target->ProcessEvent(show, &info);
    mines_notice_policy.shown(now);
    RC::Output::send<RC::LogLevel::Default>(STR("[KH02MinesNotice] SHOWN distance={}\n"),
                                            args.result);
}
} // namespace
